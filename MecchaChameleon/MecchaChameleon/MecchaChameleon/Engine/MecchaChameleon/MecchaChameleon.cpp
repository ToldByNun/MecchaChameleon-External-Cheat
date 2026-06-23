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
	this->world = memory.readMemory<uintptr_t>(memory.baseAddress + Offsets::GWorld);
	if (!this->check(this->world, "GWorld")) return false;

	this->persistentLevel = memory.readMemory<uintptr_t>(this->world + Offsets::SWorld::PersistentLevel);
	if (!this->check(this->persistentLevel, "PersistentLevel")) return false;

	this->actors = memory.readMemory<TArray>(this->persistentLevel + Offsets::SWorld::SLevel::Actors);
	if (!this->check(this->actors, "Actors")) return false;

	for (int i = 0; i < this->actors.count; i++) {
		this->actor = memory.readMemory<uintptr_t>(
			this->actors.data + i * sizeof(uintptr_t)
		);

		if (!this->check(this->actor, "Actor"))
			continue;

		this->actorClass = memory.readMemory<uintptr_t>(this->actor + Offsets::SWorld::SLevel::SActor::Class);

		if (!this->check(this->actorClass, "ActorClass"))
			continue;

		this->className = memory.readMemory<FName>(this->actorClass + Offsets::SWorld::SLevel::SActor::Name);
		if (!this->check(this->className, "ClassName"))
			continue;

		printf("Name: %s\n", this->getNameByPtr(this->actor).c_str());

		this->rootComponent = memory.readMemory<uintptr_t>(this->actor + Offsets::SWorld::SLevel::SActor::RootComponent);
		if (!this->check(this->rootComponent, "RootComponent"))
			continue;

		this->relativeLocation = memory.readMemory<FVector>(this->rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeLocation);
		if (!this->check(this->relativeLocation, "RelativeLocation"))
			continue;
		this->relativeRotation = memory.readMemory<FVector>(this->rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeRotation);
		if (!this->check(this->relativeRotation, "RelativeRotation"))
			continue;
		this->relativeScale3D = memory.readMemory<FVector>(this->rootComponent + Offsets::SWorld::SLevel::SActor::SComponent::RelativeScale3D);
		if (!this->check(this->relativeScale3D, "RelativeScale3D"))
			continue;
	}

	return true;
}