// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * 
 */
class INVENTORYPROTOTYPE_API Item
{
public:
    Item();
	Item(int ID, FString name, int weight);
	~Item();

    int ID;
    FString name;
    int weight;
    
};
