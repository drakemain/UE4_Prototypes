// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryPrototype.h"
#include "Inventory.h"

Inventory::Inventory() {
    this->maxWeight = 100;
    this->itemsInInv = 0;
}

Inventory::Inventory(int maxWeight)
{
    this->maxWeight = maxWeight;
    this->itemsInInv = 0;
}

Inventory::~Inventory()
{
}

void Inventory::add(Item* item) {
    this->inventory.Add(item);
    this->itemsInInv++;
}

FString Inventory::getName(int index) {
    return inventory[index]->name;
}

int Inventory::getWeight(int index) {
    return this->inventory[index]->weight;
}

int Inventory::getID(int index) {
    return this->inventory[index]->ID;
}

int Inventory::getStackCount() {
    return this->itemsInInv;
}