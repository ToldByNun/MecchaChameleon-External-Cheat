#pragma once

// improved using Claude Opus 4.6

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <mutex>
#include <optional>
#include <type_traits>
#include <thread>
#include <atomic>

class IManagedClass {
public:
    virtual ~IManagedClass() = default;

    virtual bool init()   { return true; }
    virtual void deinit() {}
    virtual void update() {}

    void* state = nullptr;
};

class ClassManager {
public:
    ClassManager()  = default;
    ~ClassManager() { deinit(); }

    ClassManager(const ClassManager&)            = delete;
    ClassManager& operator=(const ClassManager&) = delete;
    ClassManager(ClassManager&&)                 = delete;
    ClassManager& operator=(ClassManager&&)      = delete;

    void setState(void* state) {
        std::lock_guard lock(mutex_);
        state_ = state;
    }

    template<typename T, typename... CtorArgs>
    T* addClass(std::string_view primaryName,
                std::optional<std::string_view> alias = std::nullopt,
                CtorArgs&&... args)
    {
        static_assert(std::is_base_of_v<IManagedClass, T>,
                      "T must inherit IManagedClass");

        std::lock_guard lock(mutex_);
        assertNotInitialized("addClass");
        assertNameFree(primaryName);
        if (alias) assertNameFree(*alias);

        auto  instance   = std::make_unique<T>(std::forward<CtorArgs>(args)...);
        instance->state  = state_;
        T*    raw        = instance.get();

        entries_.push_back({ std::string(primaryName), std::move(instance) });
        std::size_t idx = entries_.size() - 1;

        nameToIndex_[std::string(primaryName)] = idx;
        if (alias)
            nameToIndex_[std::string(*alias)] = idx;

        return raw;
    }

    template<typename T>
    T* getClass(std::string_view name, T** outPtr = nullptr) {
        std::lock_guard lock(mutex_);
        T* typed = findTyped<T>(name);
        if (outPtr) *outPtr = typed;
        return typed;
    }

    template<typename T>
    T* tryGetClass(std::string_view name, T** outPtr = nullptr) noexcept {
        std::lock_guard lock(mutex_);
        auto* base  = findByName(name);
        T*    typed = base ? dynamic_cast<T*>(base) : nullptr;
        if (outPtr) *outPtr = typed;
        return typed;
    }

    [[nodiscard]] bool hasClass(std::string_view name) const noexcept {
        std::lock_guard lock(mutex_);
        return nameToIndex_.contains(std::string(name));
    }

    bool init() {
        std::lock_guard lock(mutex_);
        if (initialized_) return true;

        for (std::size_t i = 0; i < entries_.size(); ++i) {
            auto& inst  = entries_[i].instance;
            inst->state = state_;
            if (!inst->init()) {
                for (std::size_t k = i; k-- > 0;)
                    entries_[k].instance->deinit();
                return false;
            }
        }

        initialized_ = true;
        return true;
    }

    void deinit() noexcept {
        joinUpdateWorkers_();
        std::lock_guard lock(mutex_);
        if (!initialized_) return;
        for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
            it->instance->deinit();
        initialized_ = false;
    }

    void update() {
        assert(initialized_ && "ClassManager::update() called before init()");
        for (auto& entry : entries_)
            entry.instance->update();
    }

    void startUpdateLoops() {
        std::lock_guard lock(mutex_);
        assert(initialized_ && "startUpdateLoops: call init() first");
        if (!update_workers_.empty())
            return;

        update_run_.store(true, std::memory_order_release);
        update_workers_.reserve(entries_.size());
        for (auto& e : entries_) {
            IManagedClass* inst = e.instance.get();
            update_workers_.emplace_back([inst, this] {
                while (update_run_.load(std::memory_order_acquire)) {
                    inst->update();
                }
            });
        }
    }

    void stopUpdateLoops() noexcept { joinUpdateWorkers_(); }

    void removeClass(std::string_view name) {
        std::lock_guard lock(mutex_);

        auto it = nameToIndex_.find(std::string(name));
        if (it == nameToIndex_.end()) return;
        std::size_t idx = it->second;

        entries_[idx].instance->deinit();
        std::erase_if(nameToIndex_, [idx](const auto& kv){ return kv.second == idx; });
        entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(idx));
        for (auto& [key, val] : nameToIndex_)
            if (val > idx) --val;
    }

    [[nodiscard]] std::size_t count()         const noexcept { std::lock_guard l(mutex_); return entries_.size(); }
    [[nodiscard]] bool        isInitialized() const noexcept { std::lock_guard l(mutex_); return initialized_; }

    [[nodiscard]] std::vector<std::string> registeredNames() const {
        std::lock_guard lock(mutex_);
        std::vector<std::string> out;
        out.reserve(entries_.size());
        for (const auto& e : entries_) out.push_back(e.primaryName);
        return out;
    }

private:
    struct Entry {
        std::string                    primaryName;
        std::unique_ptr<IManagedClass> instance;
    };

    IManagedClass* findByName(std::string_view name) const {
        auto it = nameToIndex_.find(std::string(name));
        return (it == nameToIndex_.end()) ? nullptr : entries_[it->second].instance.get();
    }

    template<typename T>
    T* findTyped(std::string_view name) const {
        auto* base = findByName(name);
        if (!base)
            throw std::out_of_range("ClassManager: unknown class \"" + std::string(name) + "\"");
        T* typed = dynamic_cast<T*>(base);
        if (!typed)
            throw std::bad_cast();
        return typed;
    }

    void assertNotInitialized(const char* caller) const {
        if (initialized_)
            throw std::logic_error(std::string(caller) + "() called after ClassManager::init()");
    }

    void assertNameFree(std::string_view name) const {
        if (nameToIndex_.contains(std::string(name)))
            throw std::invalid_argument(
                "ClassManager: duplicate name \"" + std::string(name) + "\"");
    }

    void joinUpdateWorkers_() noexcept {
        update_run_.store(false, std::memory_order_release);
        for (auto& t : update_workers_) {
            if (t.joinable()) {
                try {
                    t.join();
                } catch (...) {
                    // preserves deinit() noexcept if join throws std::system_error
                }
            }
        }
        update_workers_.clear();
    }

    std::vector<Entry>                           entries_;
    std::unordered_map<std::string, std::size_t> nameToIndex_;
    void*                                        state_       = nullptr;
    bool                                         initialized_ = false;
    mutable std::mutex                           mutex_;
    std::atomic<bool>                            update_run_{false};
    std::vector<std::thread>                     update_workers_;
};