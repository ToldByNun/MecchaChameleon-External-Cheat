#include "MecchaChameleon.hpp"
#include "../offsets.hpp"
#include <iostream>
#include <chrono>
#include <thread>

MecchaChameleon::~MecchaChameleon() {
	stopBackgroundUpdate();
}

void MecchaChameleon::startBackgroundUpdate() {
	if (backgroundRunning)
		return;

	backgroundRunning = true;
	updateThread = std::thread(&MecchaChameleon::updateLoop, this);
}

void MecchaChameleon::stopBackgroundUpdate() {
	backgroundRunning = false;
	if (updateThread.joinable())
		updateThread.join();
}

void MecchaChameleon::updateLoop() {
	const auto interval = std::chrono::milliseconds(1);

	while (backgroundRunning) {
		update();
		std::this_thread::sleep_for(interval);
	}
}

void MecchaChameleon::getSnapshot(std::vector<TrackedActor>& outActors, FMinimalViewInfo& outViewInfo) {
	std::lock_guard<std::mutex> lock(dataMutex);
	outActors = actors;
	outViewInfo = viewInfo;
}

bool MecchaChameleon::init() {
	if (!memory.attachToProcess("PenguinHotel-Win64-Shipping.exe")) {
		std::cout << "[-] Failed to attach to process.\n";
		return false;
	}

	this->check(memory.baseAddress, "BaseAddress");
	return true;
}

bool MecchaChameleon::resolveChain() {
	uintptr_t world = memory.readMemory<uintptr_t>(memory.baseAddress + Offsets::GWorld);
	if (!this->check(world, "GWorld")) return false;

	// Persistent Level
	uintptr_t persistentLevel = memory.readMemory<uintptr_t>(world + Offsets::SWorld::PersistentLevel);
	if (!this->check(persistentLevel, "PersistentLevel")) return false;

	TArray actors = memory.readMemory<TArray>(persistentLevel + Offsets::SWorld::SLevel::Actors);
	if (!this->check(actors, "Actors")) return false;

	for (int i = 0; i < actors.count; i++) {
		uintptr_t actor = memory.readMemory<uintptr_t>(
			actors.data + i * sizeof(uintptr_t)
		);

		if (!this->check(actor, "Actor"))
			continue;

		uintptr_t actorClass = memory.readMemory<uintptr_t>(actor + Offsets::SWorld::SLevel::SActor::Class);

		if (!this->check(actorClass, "ActorClass"))
			continue;

		FName className = memory.readMemory<FName>(actorClass + Offsets::SWorld::SLevel::SActor::Name);
		if (!this->check(className, "ClassName"))
			continue;

		printf("Name: %s\n", this->getNameByPtr(actor).c_str());

		uintptr_t rootComponent = memory.readMemory<uintptr_t>(actor + Offsets::SWorld::SLevel::SActor::RootComponent);
		if (!this->check(rootComponent, "RootComponent"))
			continue;

		FVector relativeLocation = memory.readMemory<FVector>(rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeLocation);
		if (!this->check(relativeLocation, "RelativeLocation"))
			continue;
		FVector relativeRotation = memory.readMemory<FVector>(rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeRotation);
		if (!this->check(relativeRotation, "RelativeRotation"))
			continue;
		FVector relativeScale3D = memory.readMemory<FVector>(rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeScale3D);
		if (!this->check(relativeScale3D, "RelativeScale3D"))
			continue;
	}

	// Game Instance
	uintptr_t gameInstance = memory.readMemory<uintptr_t>(world + Offsets::SWorld::OwningGameInstance);
	if (!this->check(gameInstance, "GameInstance")) return false;

	TArray localPlayers = memory.readMemory<TArray>(gameInstance + Offsets::SWorld::SGameInstance::LocalPlayers);
	if (!this->check(localPlayers, "LocalPlayers")) return false;

	uintptr_t localPlayer = memory.readMemory<uintptr_t>(localPlayers.data);
	if (!this->check(localPlayer, "LocalPlayer")) return false;

	uintptr_t playerController = memory.readMemory<uintptr_t>(localPlayer + Offsets::SWorld::SGameInstance::SLocalPlayers::PlayerController);
	if (!this->check(playerController, "PlayerController")) return false;

	uintptr_t cameraManager = memory.readMemory<uintptr_t>(playerController + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::PlayerCameraManager);
	if (!this->check(cameraManager, "CameraManager")) return false;

	FMinimalViewInfo viewInfo = memory.readMemory<FMinimalViewInfo>(cameraManager + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPlayerCameraManager::CameraInfo);
	if (!this->check(viewInfo, "ViewInfo")) return false;

	return true;
}

bool MecchaChameleon::update() {
	std::vector<TrackedActor> newActors;

	auto logFail = [](const char* reason) {
		static auto last = std::chrono::steady_clock::now();
		const auto now = std::chrono::steady_clock::now();
		if (now - last < std::chrono::seconds(2))
			return;
		last = now;
		std::cout << "[update] fehlgeschlagen: " << reason << "\n";
	};

	uintptr_t world = memory.readMemory<uintptr_t>(memory.baseAddress + Offsets::GWorld);
	if (!world) {
		logFail("GWorld ist 0");
		return false;
	}

	uintptr_t persistentLevel = memory.readMemory<uintptr_t>(world + Offsets::SWorld::PersistentLevel);
	if (!persistentLevel) {
		logFail("PersistentLevel ist 0");
		return false;
	}

	TArray actorArray = memory.readMemory<TArray>(persistentLevel + Offsets::SWorld::SLevel::Actors);
	if (!actorArray.data || actorArray.count <= 0 || actorArray.count > 1000000) {
		logFail("Actor-Array ungueltig oder leer");
		return false;
	}

	newActors.reserve(actorArray.count);

	for (int i = 0; i < actorArray.count; i++) {
		uintptr_t actor = memory.readMemory<uintptr_t>(actorArray.data + i * sizeof(uintptr_t));
		if (!actor) continue;

		uintptr_t rootComponent = memory.readMemory<uintptr_t>(actor + Offsets::SWorld::SLevel::SActor::RootComponent);
		if (!rootComponent) continue;

		FVector location = memory.readMemory<FVector>(
			rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeLocation
		);

		if (location.x == 0 && location.y == 0 && location.z == 0)
			continue;

		newActors.push_back({ location });
	}

	uintptr_t gameInstance = memory.readMemory<uintptr_t>(world + Offsets::SWorld::OwningGameInstance);
	if (!gameInstance) {
		logFail("OwningGameInstance ist 0");
		return false;
	}

	TArray localPlayers = memory.readMemory<TArray>(gameInstance + Offsets::SWorld::SGameInstance::LocalPlayers);
	if (!localPlayers.data || localPlayers.count <= 0) {
		logFail("LocalPlayers leer");
		return false;
	}

	uintptr_t localPlayer = memory.readMemory<uintptr_t>(localPlayers.data);
	if (!localPlayer) {
		logFail("LocalPlayer ist 0");
		return false;
	}

	uintptr_t playerController = memory.readMemory<uintptr_t>(
		localPlayer + Offsets::SWorld::SGameInstance::SLocalPlayers::PlayerController
	);
	if (!playerController) {
		logFail("PlayerController ist 0");
		return false;
	}

	uintptr_t cameraManager = memory.readMemory<uintptr_t>(
		playerController + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::PlayerCameraManager
	);
	if (!cameraManager) {
		logFail("PlayerCameraManager ist 0");
		return false;
	}

	FMinimalViewInfo newViewInfo = memory.readMemory<FMinimalViewInfo>(
		cameraManager + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPlayerCameraManager::CameraInfo
	);

	{
		std::lock_guard<std::mutex> lock(dataMutex);
		actors = std::move(newActors);
		viewInfo = newViewInfo;
	}

	{
		static auto last = std::chrono::steady_clock::now();
		const auto now = std::chrono::steady_clock::now();
		if (now - last >= std::chrono::seconds(2)) {
			last = now;
			std::cout << "[update] ok | level actors=" << actorArray.count
				<< " | tracked=" << actors.size()
				<< " | cam=(" << (int)viewInfo.Location.x << ", " << (int)viewInfo.Location.y << ", " << (int)viewInfo.Location.z << ")"
				<< " fov=" << viewInfo.FOV << "\n";
		}
	}

	return true;
}