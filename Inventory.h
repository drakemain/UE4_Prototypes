// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Item.h"

/**
 * 
 */
class INVENTORYPROTOTYPE_API Inventory
{
public:
    Inventory();
	Inventory(int maxWeight);
	~Inventory();

    void add(Item* item);
    FString getName(int index);
    int getWeight(int index);
    int getID(int index);
    int getStackCount();

private:
    int maxWeight;
    int currentWeight;
    int itemsInInv;

    TArray<Item*> inventory;

    Item testItem;
};
