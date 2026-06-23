#include "MecchaChameleon.hpp"
#include "../offsets.hpp"
#include <iostream>
#include <chrono>
#include <thread>

MecchaChameleon::~MecchaChameleon() {
	stopBackgroundUpdate();
}

void MecchaChameleon::startBackgroundUpdate() {
	if (this->backgroundRunning)
		return;

	this->backgroundRunning = true;
	this->updateThread = std::thread(&MecchaChameleon::updateLoop, this);
}

void MecchaChameleon::stopBackgroundUpdate() {
	this->backgroundRunning = false;
	if (this->updateThread.joinable())
		this->updateThread.join();
}

void MecchaChameleon::updateLoop() {
	const auto interval = std::chrono::milliseconds(1);

	while (this->backgroundRunning) {
		update();
		std::this_thread::sleep_for(interval);
	}
}

void MecchaChameleon::getSnapshot(std::vector<TrackedActor>& outActors, FMinimalViewInfo& outViewInfo) {
	std::lock_guard<std::mutex> lock(this->dataMutex);
	outActors = this->actors;
	outViewInfo = this->viewInfo;
}

bool MecchaChameleon::init() {
	if (!memory.attachToProcess("PenguinHotel-Win64-Shipping.exe")) {
		std::cout << "[-] Failed to attach to process.\n";
		return false;
	}

	if (!this->check(memory.baseAddress, "BaseAddress"))
		return false;

	if (!resolveChain())
		return false;

	chainResolved = true;
	return true;
}

bool MecchaChameleon::resolveWorld() {
	this->world = this->memory.readMemory<uintptr_t>(this->memory.baseAddress + Offsets::GWorld);
	return this->check(this->world, "GWorld");
}

bool MecchaChameleon::resolvePersistentLevel() {
	this->persistentLevel = this->memory.readMemory<uintptr_t>(this->world + Offsets::SWorld::PersistentLevel);
	return this->check(this->persistentLevel, "PersistentLevel");
}

bool MecchaChameleon::resolveGameInstance() {
	this->gameInstance = this->memory.readMemory<uintptr_t>(this->world + Offsets::SWorld::OwningGameInstance);
	return this->check(this->gameInstance, "GameInstance");
}

bool MecchaChameleon::resolveLocalPlayer() {
	TArray localPlayers = this->memory.readMemory<TArray>(this->gameInstance + Offsets::SWorld::SGameInstance::LocalPlayers);
	if (!this->check(localPlayers, "LocalPlayers"))
		return false;

	this->localPlayer = this->memory.readMemory<uintptr_t>(localPlayers.data);
	return this->check(this->localPlayer, "LocalPlayer");
}

bool MecchaChameleon::resolvePlayerController() {
	this->playerController = this->memory.readMemory<uintptr_t>(
		this->localPlayer + Offsets::SWorld::SGameInstance::SLocalPlayers::PlayerController
	);
	return this->check(this->playerController, "PlayerController");
}

bool MecchaChameleon::resolveCameraManager() {
	this->cameraManager = this->memory.readMemory<uintptr_t>(
		this->playerController + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::PlayerCameraManager
	);
	if (!this->check(this->cameraManager, "CameraManager"))
		return false;

	FMinimalViewInfo cameraViewInfo = this->memory.readMemory<FMinimalViewInfo>(
		this->cameraManager + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPlayerCameraManager::CameraInfo
	);
	return this->check(cameraViewInfo, "ViewInfo");
}

bool MecchaChameleon::resolveGameState() {
	this->gameState = this->memory.readMemory<uintptr_t>(this->world + Offsets::SWorld::GameState);
	return this->check(this->gameState, "GameState");
}

bool MecchaChameleon::validatePlayerArray() {
	TArray playerArray = memory.readMemory<TArray>(gameState + Offsets::SWorld::SGameState::PlayerArray);
	if (!this->check(playerArray, "PlayerArray"))
		return false;

	using SPawn = Offsets::SWorld::SGameState::SPlayerArray::SPawn;
	using SMesh = Offsets::SWorld::SGameState::SPlayerArray::SMesh;

	for (int i = 0; i < playerArray.count; i++) {
		uintptr_t playerState = this->memory.readMemory<uintptr_t>(playerArray.data + i * sizeof(uintptr_t));
		if (!this->check(playerState, "PlayerState"))
			continue;

		uintptr_t pawn = this->memory.readMemory<uintptr_t>(playerState + Offsets::SWorld::SGameState::SPlayerArray::Pawn);
		if (!this->check(pawn, "Pawn"))
			continue;

		uintptr_t mesh = this->memory.readMemory<uintptr_t>(pawn + SPawn::Mesh);
		if (!this->check(mesh, "Mesh"))
			continue;

		uintptr_t skeletalMesh = this->memory.readMemory<uintptr_t>(mesh + SMesh::SkeletalMesh);
		if (!this->check(skeletalMesh, "SkeletalMesh"))
			continue;

		TArray boneSpace = this->memory.readMemory<TArray>(mesh + SMesh::BoneSpaceTransforms);
		if (!this->check(boneSpace, "BoneSpaceTransforms"))
			continue;

		TArray componentSpace = this->memory.readMemory<TArray>(mesh + SMesh::ComponentSpaceTransforms);
		if (!this->check(componentSpace, "ComponentSpaceTransforms"))
			continue;

		printf("mesh=%s class=%s skeletal=%s bones=%d comp=%d\n",
			getNameByPtr(mesh).c_str(),
			getNameByPtr(this->memory.readMemory<uintptr_t>(mesh + SMesh::Class)).c_str(),
			getNameByPtr(skeletalMesh).c_str(),
			boneSpace.count,
			componentSpace.count
		);
	}

	return true;
}

bool MecchaChameleon::resolveChain() {
	if (!resolveWorld()) return false;
	if (!resolvePersistentLevel()) return false;
	if (!resolveGameInstance()) return false;
	if (!resolveLocalPlayer()) return false;
	if (!resolvePlayerController()) return false;
	if (!resolveCameraManager()) return false;
	if (!resolveGameState()) return false;
	if (!validatePlayerArray()) return false;
	return true;
}

bool MecchaChameleon::update() {
	if (!this->chainResolved)
		return false;

	std::vector<TrackedActor> newActors;

	auto logFail = [](const char* reason) {
		static auto last = std::chrono::steady_clock::now();
		const auto now = std::chrono::steady_clock::now();
		if (now - last < std::chrono::seconds(2))
			return;
		last = now;
		std::cout << "[update] error: " << reason << "\n";
	};

	if (!this->gameState || !this->cameraManager) {
		logFail("chain pointers invalid");
		return false;
	}

	FMinimalViewInfo newViewInfo = memory.readMemory<FMinimalViewInfo>(
		this->cameraManager + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPlayerCameraManager::CameraInfo
	);

	TArray playerArray = this->memory.readMemory<TArray>(this->gameState + Offsets::SWorld::SGameState::PlayerArray);
	if (!playerArray.data || playerArray.count <= 0 || playerArray.count > 1000000) {
		logFail("PlayerArray empty");
		return false;
	}

	newActors.reserve(playerArray.count);

	using SPawn = Offsets::SWorld::SGameState::SPlayerArray::SPawn;
	using SMesh = Offsets::SWorld::SGameState::SPlayerArray::SMesh;

	for (int i = 0; i < playerArray.count; i++) {
		uintptr_t playerState = this->memory.readMemory<uintptr_t>(playerArray.data + i * sizeof(uintptr_t));
		if (!playerState)
			continue;

		uintptr_t pawn = this->memory.readMemory<uintptr_t>(playerState + Offsets::SWorld::SGameState::SPlayerArray::Pawn);
		if (!pawn)
			continue;

		uintptr_t mesh = this->memory.readMemory<uintptr_t>(pawn + SPawn::Mesh);
		if (mesh) {
			this->memory.readMemory<uintptr_t>(mesh + SMesh::SkeletalMesh);
			this->memory.readMemory<TArray>(mesh + SMesh::BoneSpaceTransforms);
			this->memory.readMemory<TArray>(mesh + SMesh::ComponentSpaceTransforms);
		}

		uintptr_t rootComponent = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SLevel::SActor::RootComponent);
		if (!rootComponent)
			continue;

		FVector location = this->memory.readMemory<FVector>(
			rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeLocation
		);

		if (location.x == 0 && location.y == 0 && location.z == 0)
			continue;

		newActors.push_back({ location });
	}

	{
		std::lock_guard<std::mutex> lock(this->dataMutex);
		this->actors = std::move(newActors);
		this->viewInfo = newViewInfo;
	}

	return true;
}
