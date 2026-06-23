#include "MecchaChameleon.hpp"
#include "../offsets.hpp"
#include <iostream>
#include <chrono>
#include <thread>

bool MecchaChameleon::init() {
	if (!memory.attachToProcess("PenguinHotel-Win64-Shipping.exe")) {
		std::cout << "[-] Failed to attach to process.\n";
		return false;
	}

	this->check(memory.baseAddress, "BaseAddress");

	while (!this->resolveChain()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

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