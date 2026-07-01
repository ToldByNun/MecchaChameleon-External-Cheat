#include "MecchaChameleon.hpp"

#include "../../Manager/Globals/Globals.hpp"
#include "../offsets.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

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

bool MecchaChameleon::tests() {
	uintptr_t pawn = this->memory.readMemory<uintptr_t>(this->playerController + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::AcknowledgedPawn);
	if (!pawn) return false;

	uintptr_t characterMovementComponent = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPawn::CharacterMovementComponent);
	if (!characterMovementComponent) return false;

	this->memory.writeMemory<float>(characterMovementComponent + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPawn::SCharacterMovementComponent::MaxAcceleration, 6000.0f);
	this->memory.writeMemory<float>(characterMovementComponent + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPawn::SCharacterMovementComponent::MaxWalkSpeed, 6000.0f);
	this->memory.writeMemory<float>(characterMovementComponent + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPawn::SCharacterMovementComponent::MinAnalogWalkSpeed, 6000.0f);

	return true;
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

		uintptr_t headPosition = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SGameState::SPlayerArray::SPawn::HeadPosition);
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

FVector MecchaChameleon::readRelativeLocation(uintptr_t rootComponent) {
	if (!this->isValidPtr(rootComponent)) return {};

	return this->memory.readMemory<FVector>(rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeLocation);
}

FVector MecchaChameleon::readWorldLocation(uintptr_t pawn) {
	if (!this->isValidPtr(pawn)) return {};

	uintptr_t root = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SLevel::SActor::RootComponent);

	if (!this->isValidPtr(root)) return {};

	return this->memory.readMemory<FVector>(root + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPawn::SRootComponent::ComponentToWorldTranslation);
}

bool MecchaChameleon::tryReadMoverSyncLocation(uintptr_t pawn, const FVector& groundTruth, FVector& outSync) {
	constexpr double kTol = 0.75;

	if (!this->isValidPtr(pawn)) return false;

	uintptr_t mover = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SLevel::SActor::ExtendedPhysicsCharacterMoverComponent);
	if (!this->isValidPtr(mover)) return false;

	uintptr_t heapBase = this->memory.readMemory<uintptr_t>(mover + Offsets::SWorld::SLevel::SActor::SMoverComponent::SyncHeapPointer);
	if (!this->isValidPtr(heapBase)) return false;

	std::vector<uint8_t> heap(Offsets::SWorld::SLevel::SActor::SMoverComponent::SSyncHeap::ScanWindowSize);
	if (!this->memory.readRawMemory(heapBase, heap.data(), heap.size())) return false;

	uintptr_t matchAddress = 0;

	for (size_t off = 0; off + 0x20 <= heap.size(); off += 8) {
		double x{}, y{}, z{};
		std::memcpy(&x, heap.data() + off + Offsets::SWorld::SLevel::SActor::SMoverComponent::SSyncHeap::SDefaultSyncState::LocationX, sizeof(double));
		std::memcpy(&y, heap.data() + off + Offsets::SWorld::SLevel::SActor::SMoverComponent::SSyncHeap::SDefaultSyncState::LocationY, sizeof(double));
		std::memcpy(&z, heap.data() + off + Offsets::SWorld::SLevel::SActor::SMoverComponent::SSyncHeap::SDefaultSyncState::LocationZ, sizeof(double));

		if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) continue;
		if (std::abs(x) > 5'000'000.0 || std::abs(y) > 5'000'000.0 || std::abs(z) > 5'000'000.0) continue;
		if (abs(x - groundTruth.x) > kTol || abs(y - groundTruth.y) > kTol || abs(z - groundTruth.z) > kTol) continue;
		if (matchAddress != 0) return false;

		matchAddress = heapBase + off;
	}

	if (!matchAddress) return false;

	outSync.x = this->memory.readMemory<double>(matchAddress + Offsets::SWorld::SLevel::SActor::SMoverComponent::SSyncHeap::SDefaultSyncState::LocationX);
	outSync.y = this->memory.readMemory<double>(matchAddress + Offsets::SWorld::SLevel::SActor::SMoverComponent::SSyncHeap::SDefaultSyncState::LocationY);
	outSync.z = this->memory.readMemory<double>(matchAddress + Offsets::SWorld::SLevel::SActor::SMoverComponent::SSyncHeap::SDefaultSyncState::LocationZ);

	return true;
}

PlayerLocationRead MecchaChameleon::readPlayerLocation(uintptr_t pawn) {
	PlayerLocationRead result{};

	if (!isValidPtr(pawn)) return result;

	uintptr_t root = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SLevel::SActor::RootComponent);
	if (!isValidPtr(root)) return result;

	result.relativeLocation = readRelativeLocation(root);
	result.worldLocation = readWorldLocation(pawn);

	result.hasWorldLocation = std::isfinite(result.worldLocation.x) && std::isfinite(result.worldLocation.y) && std::isfinite(result.worldLocation.z) && !(result.worldLocation.x == 0.0 && result.worldLocation.y == 0.0 && result.worldLocation.z == 0.0);
	if (result.hasWorldLocation) result.hasMoverSync = tryReadMoverSyncLocation(pawn, result.worldLocation, result.moverSyncLocation);
	
	return result;
}

FTransformD MecchaChameleon::readTransform(uintptr_t address)
{
	FTransformD transform{};

	if (!isValidPtr(address)) return transform;

	transform.rotation = this->memory.readMemory<FQuat>(address + 0x00);
	transform.translation = this->memory.readMemory<FVectorD>(address + 0x20);
	transform.scale3d = this->memory.readMemory<FVectorD>(address + 0x40);

	return transform;
}

FVectorD MecchaChameleon::TransformPosition(const FTransformD& transform, const FVectorD& position)
{
	FVectorD scaled{
		position.x * transform.scale3d.x,
		position.y * transform.scale3d.y,
		position.z * transform.scale3d.z
	};

	FVectorD rotated = RotateVector(transform.rotation, scaled);

	return {
		rotated.x + transform.translation.x,
		rotated.y + transform.translation.y,
		rotated.z + transform.translation.z
	};
}

bool MecchaChameleon::isValidPtr(uintptr_t ptr) {
	constexpr uintptr_t minPtr = 0x10000ULL;
	constexpr uintptr_t maxPtr = 0x00007FFFFFFFFFFFULL;

	return ptr >= minPtr && ptr <= maxPtr;
}

bool MecchaChameleon::isValidTArray(const TArray& array, int32_t minCount, int32_t maxCount) {
	return isValidPtr(array.data) && array.count >= minCount && array.count <= maxCount;
}

TArray MecchaChameleon::readBoneTransforms(uintptr_t mesh) {
	TArray transforms = this->memory.readMemory<TArray>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::ComponentSpaceTransforms);

	if (!isValidTArray(transforms, kSkeletonBoneCount, 256)) {
		transforms.data = this->memory.readMemory<uintptr_t>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::CachedComponentSpaceTransforms);
		transforms.count = this->memory.readMemory<int32_t>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::CachedComponentSpaceTransforms + 0x8);
	}

	return transforms;
}

std::vector<FVectorD> MecchaChameleon::readSkeletonBones(uintptr_t mesh) {
	std::vector<FVectorD> bones;

	if (!isValidPtr(mesh)) return bones;

	TArray transforms = this->readBoneTransforms(mesh);

	if (!isValidTArray(transforms, kSkeletonBoneCount, 256)) return bones;

	FTransformD componentToWorld = this->readTransform(mesh + Offsets::SWorld::SGameState::SPlayerArray::SMesh::ComponentToWorld);
	const int boneCount = min(transforms.count, kSkeletonBoneCount);

	bones.reserve(boneCount);

	for (int boneIndex = 0; boneIndex < boneCount; boneIndex++) {
		uintptr_t boneAddress = transforms.data + static_cast<uintptr_t>(boneIndex) * Offsets::SWorld::SGameState::SPlayerArray::SMesh::BoneTransformStride;

		if (!isValidPtr(boneAddress)) break;

		FTransformD boneComponent = this->readTransform(boneAddress);
		bones.push_back(this->TransformPosition(componentToWorld, boneComponent.translation));
	}

	return bones;
}

bool MecchaChameleon::tryReadTrackedActor(uintptr_t playerState, uintptr_t localPawn, PlayerRole localRole, TrackedActor& outActor) {
	if (!isValidPtr(playerState)) return false;

	std::string playerName = this->memory.readFString(playerState + Offsets::SWorld::SGameState::SPlayerArray::PlayerName);

	uintptr_t pawn = this->memory.readMemory<uintptr_t>(playerState + Offsets::SWorld::SGameState::SPlayerArray::Pawn);
	if (!isValidPtr(pawn)) return false;

	PlayerRole playerRole = this->getRoleFromClass(pawn);

	uintptr_t headPosition = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SGameState::SPlayerArray::SPawn::HeadPosition);
	if (!isValidPtr(headPosition)) return false;

	double headRadius = this->memory.readMemory<double>(headPosition + Offsets::SWorld::SGameState::SPlayerArray::SPawn::SHeadPosition::HeadRadius);
	double playerSize = this->memory.readMemory<double>(headPosition + Offsets::SWorld::SGameState::SPlayerArray::SPawn::SHeadPosition::PlayerSize);

	uintptr_t mesh = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SGameState::SPlayerArray::SPawn::Mesh);
	std::vector<FVectorD> bones = this->readSkeletonBones(mesh);

	uintptr_t rootComponent = this->memory.readMemory<uintptr_t>(pawn + Offsets::SWorld::SLevel::SActor::RootComponent);
	if (!isValidPtr(rootComponent)) return false;

	PlayerLocationRead location = this->readPlayerLocation(pawn);
	if (!location.hasWorldLocation) return false;

	outActor = {
		location.worldLocation,
		bones,
		headRadius,
		playerSize,
		playerName,
		localRole == playerRole && playerRole != PlayerRole::UNKNOWN,
		pawn == localPawn,
		pawn
	};

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
	if (!this->gWorldAddress) return false;

	uintptr_t currentWorld = this->memory.readMemory<uintptr_t>(this->gWorldAddress);
	if (!currentWorld) return false;

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

	if (!refresh()) this->chainResolved = resolveChain() && refresh();
}

// Testing. nothing serious yet
void MecchaChameleon::applyFlatChams(uintptr_t pawn) {
	if (!pawn) return;

	uintptr_t mesh = this->memory.readMemory<uintptr_t>(
		pawn + Offsets::SWorld::SGameState::SPlayerArray::SPawn::Mesh
	);

	if (!mesh) return;

	TArray overrideMaterials = this->memory.readMemory<TArray>(mesh + Offsets::SWorld::SGameState::SPlayerArray::SPawn::SMesh::OverrideMaterials);

	if (!overrideMaterials.data || overrideMaterials.count <= 0 || overrideMaterials.count > 8) return;

	uintptr_t slot = overrideMaterials.data;
	uintptr_t material = this->memory.readMemory<uintptr_t>(slot);

	if (!material) return;

	std::string materialName = this->getNameByPtr(material);

	if (materialName == "M_PaintTarget") return;

	uintptr_t parentMaterial = this->memory.readMemory<uintptr_t>(material + Offsets::SWorld::SGameState::SPlayerArray::SPawn::SMesh::SMaterial::ParentMaterial);

	if (!parentMaterial) return;

	std::string parentName = this->getNameByPtr(parentMaterial);

	if (parentName != "M_PaintTarget") return;

	this->memory.writeMemory<uintptr_t>(slot, parentMaterial);
}

bool MecchaChameleon::refresh() {
	if (!this->chainResolved) return false;
	if (!isValidPtr(this->gameState) || !isValidPtr(this->cameraManager) || !isValidPtr(this->playerController)) return false;

	uintptr_t localPawn = this->memory.readMemory<uintptr_t>(
		this->playerController + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::LocalPawn
	);

	this->isValidPtr(localPawn) ? this->localPlayerWorldLocation = readWorldLocation(localPawn) : this->localPlayerWorldLocation = {};

	PlayerRole localRole = this->getRoleFromClass(localPawn);

	FMinimalViewInfo newViewInfo = this->memory.readMemory<FMinimalViewInfo>(
		this->cameraManager + Offsets::SWorld::SGameInstance::SLocalPlayers::SPlayerController::SPlayerCameraManager::CameraInfo
	);

	TArray playerArray = this->memory.readMemory<TArray>(this->gameState + Offsets::SWorld::SGameState::PlayerArray);
	if (!isValidTArray(playerArray, 1, kMaxTrackedPlayers)) {
		std::lock_guard<std::mutex> lock(this->dataMutex);
		this->actors.clear();
		this->viewInfo = newViewInfo;
		return false;
	}

	std::vector<TrackedActor> newActors;
	newActors.reserve(playerArray.count);

	for (int playerIndex = 0; playerIndex < playerArray.count; playerIndex++) {
		uintptr_t playerState = this->memory.readMemory<uintptr_t>(playerArray.data + playerIndex * sizeof(uintptr_t));
		TrackedActor actor{};

		if (!this->tryReadTrackedActor(playerState, localPawn, localRole, actor)) continue;

		newActors.push_back(actor);
	}

	std::lock_guard<std::mutex> lock(this->dataMutex);
	this->actors = std::move(newActors);
	this->viewInfo = newViewInfo;

	return true;
}
