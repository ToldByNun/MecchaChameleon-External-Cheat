#include "MecchaChameleon.hpp"

#include "../../Manager/Globals/Globals.hpp"
#include "../offsets.hpp"
#include <iostream>
#include <chrono>
#include <thread>

MecchaChameleon::~MecchaChameleon() {
	stopBackgroundUpdate();
}

void MecchaChameleon::deinit() {
	stopBackgroundUpdate();
	chainResolved = false;
	gWorldAddress = 0;
	world = 0;
	names = 0;
	persistentLevel = 0;
	gameInstance = 0;
	localPlayer = 0;
	playerController = 0;
	cameraManager = 0;
	gameState = 0;
	memory.detachFromProcess();
}

void MecchaChameleon::startBackgroundUpdate() {
	if (this->backgroundRunning) return;

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

	if (!this->check(memory.baseAddress, "BaseAddress")) return false;

	AobResult worldResult = this->memory.resolveAob(
		Offsets::AOBs::GWorldAOB,
		Offsets::AOBs::GWorldInstructionOffset,
		RipMode::Mov
	);
	this->gWorldAddress = worldResult.target;
	this->world = worldResult.value;
	Offsets::GWorld = this->gWorldAddress;

	this->names = this->memory.resolveAob(
		Offsets::AOBs::GNamesAOB,
		Offsets::AOBs::GNamesInstructionOffset,
		RipMode::Lea
	).value; Offsets::GNames = this->names;

	if (!resolveChain()) return false;

	validatePlayerArray();
	chainResolved = true;
	return true;
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
	if (!this->check(localPlayers, "LocalPlayers")) return false;

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
	if (!this->check(this->cameraManager, "CameraManager")) return false;

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
	if (!this->check(playerArray, "PlayerArray")) return false;

	for (int i = 0; i < playerArray.count; i++) {
		uintptr_t playerState = this->memory.readMemory<uintptr_t>(playerArray.data + i * sizeof(uintptr_t));
		if (!this->check(playerState, "PlayerState")) continue;

		uintptr_t pawn = this->memory.readMemory<uintptr_t>(playerState + Offsets::SWorld::SGameState::SPlayerArray::Pawn);
		if (!this->check(pawn, "Pawn")) continue;

		uintptr_t headPosition = this->memory.readMemory<uintptr_t>(pawn + 0x400);
		if (!this->check(headPosition, "HeadPosition")) continue;

		uintptr_t mesh = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SGameState::SPlayerArray::SPawn::Mesh);
		if (!this->check(mesh, "Mesh")) continue;

		uintptr_t skeletalMesh = this->memory.readMemory<uintptr_t>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::SkeletalMesh);
		if (!this->check(skeletalMesh, "SkeletalMesh")) continue;

		TArray boneSpace = this->memory.readMemory<TArray>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::BoneSpaceTransforms);
		if (!this->check(boneSpace, "BoneSpaceTransforms")) continue;

		TArray componentSpace = this->memory.readMemory<TArray>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::ComponentSpaceTransforms);
		if (!this->check(componentSpace, "ComponentSpaceTransforms")) continue;
	}

	return true;
}

bool MecchaChameleon::resolveChain() {
	if (!resolvePersistentLevel()) return false;
	if (!resolveGameInstance()) return false;
	if (!resolveLocalPlayer()) return false;
	if (!resolvePlayerController()) return false;
	if (!resolveCameraManager()) return false;
	if (!resolveGameState()) return false;
	return true;
}

void MecchaChameleon::clearSnapshot() {
	std::lock_guard<std::mutex> lock(this->dataMutex);
	this->actors.clear();
	this->viewInfo = {};
}

bool MecchaChameleon::updateWorldPointer() {
	if (!this->gWorldAddress)
		return false;

	uintptr_t currentWorld = this->memory.readMemory<uintptr_t>(this->gWorldAddress);
	if (!currentWorld)
		return false;

	if (currentWorld != this->world) {
		this->world = currentWorld;
		this->chainResolved = false;
		this->persistentLevel = 0;
		this->gameInstance = 0;
		this->localPlayer = 0;
		this->playerController = 0;
		this->cameraManager = 0;
		this->gameState = 0;
		clearSnapshot();
	}

	return true;
}

void MecchaChameleon::update() {
	if (!updateWorldPointer()) {
		this->chainResolved = false;
		clearSnapshot();
		return;
	}

	if (!this->chainResolved && !resolveChain()) {
		clearSnapshot();
		return;
	}

	this->chainResolved = true;

	if (!refresh())
		this->chainResolved = resolveChain() && refresh();
}

// Testing. nothing serious yet
void MecchaChameleon::applyFlatChams(uintptr_t pawn) {
	if (!pawn)
		return;

	uintptr_t mesh = this->memory.readMemory<uintptr_t>(
		pawn + Offsets::SWorld::SGameState::SPlayerArray::SPawn::Mesh
	);

	if (!mesh)
		return;

	TArray overrideMaterials = this->memory.readMemory<TArray>(mesh + 0x520);

	if (!overrideMaterials.data || overrideMaterials.count <= 0 || overrideMaterials.count > 8)
		return;

	uintptr_t slot = overrideMaterials.data;
	uintptr_t material = this->memory.readMemory<uintptr_t>(slot);

	if (!material)
		return;

	std::string materialName = this->getNameByPtr(material);

	if (materialName == "M_PaintTarget")
		return;

	uintptr_t parentMaterial = this->memory.readMemory<uintptr_t>(material + 0x128);

	if (!parentMaterial)
		return;

	std::string parentName = this->getNameByPtr(parentMaterial);

	if (parentName != "M_PaintTarget")
		return;

	this->memory.writeMemory<uintptr_t>(slot, parentMaterial);
}

bool MecchaChameleon::refresh() {
	if (!this->chainResolved) return false;

	std::vector<TrackedActor> newActors;

	uintptr_t localPawn = memory.readMemory<uintptr_t>(this->playerController + 0x2E8);

	PlayerRole localRole = this->getRoleFromClass(localPawn);

	if (!this->gameState || !this->cameraManager) return false;

	FMinimalViewInfo newViewInfo = memory.readMemory<FMinimalViewInfo>(this->cameraManager + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPlayerCameraManager::CameraInfo);

	TArray playerArray = this->memory.readMemory<TArray>(this->gameState + Offsets::SWorld::SGameState::PlayerArray);
	if (!playerArray.data || playerArray.count <= 0 || playerArray.count > 1000000) {
		std::lock_guard<std::mutex> lock(this->dataMutex);
		this->actors.clear();
		this->viewInfo = newViewInfo;
		return false;
	}

	newActors.reserve(playerArray.count);

	for (int i = 0; i < playerArray.count; i++) {
		uintptr_t playerState = this->memory.readMemory<uintptr_t>(playerArray.data + i * sizeof(uintptr_t));
		if (!playerState) continue;

		std::string playerName = this->memory.readFString(playerState + Offsets::SWorld::SGameState::SPlayerArray::PlayerName);

		uintptr_t pawn = this->memory.readMemory<uintptr_t>(playerState + Offsets::SWorld::SGameState::SPlayerArray::Pawn);
		if (!pawn) continue;

		PlayerRole playerRole = this->getRoleFromClass(pawn);

		uintptr_t headPosition = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SGameState::SPlayerArray::SPawn::HeadPosition);
		if (!headPosition) continue;

		double headRadius = this->memory.readMemory<double>(headPosition + Offsets::SWorld::SGameState::SPlayerArray::SPawn::SHeadPosition::HeadRadius);

		double playerSize = this->memory.readMemory<double>(headPosition + Offsets::SWorld::SGameState::SPlayerArray::SPawn::SHeadPosition::PlayerSize);

		uintptr_t mesh = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SGameState::SPlayerArray::SPawn::Mesh);
		if (mesh) {
			//this->memory.readMemory<uintptr_t>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::SkeletalMesh);
			this->memory.readMemory<TArray>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::BoneSpaceTransforms);
			this->memory.readMemory<TArray>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::ComponentSpaceTransforms);
		}

		uintptr_t rootComponent = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SLevel::SActor::RootComponent);
		if (!rootComponent) continue;

		FVector location = this->memory.readMemory<FVector>(rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeLocation);

		if (location.x == 0 && location.y == 0 && location.z == 0) continue;

		newActors.push_back(
			{ 
				location,
				headRadius,
				playerSize,
				playerName,
				localRole == playerRole && playerRole != PlayerRole::UNKNOWN,
				pawn == localPawn,
				pawn
			}
		);
	}

	std::lock_guard<std::mutex> lock(this->dataMutex);
	this->actors = std::move(newActors);
	this->viewInfo = newViewInfo;

	return true;
}
