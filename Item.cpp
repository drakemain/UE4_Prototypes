// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryPrototype.h"
#include "Item.h"

Item::Item(){}

Item::Item(int ID, FString name, int weight)
{
    this->ID = ID;
    this->name = name;
    this->weight = weight;
}

Item::~Item()
{
}
