# UE4_Prototypes

This is a default character class from the first-person template in which I've implemeted a sprinting 
system and a simple inventory system.

The sprint system includes stamina, which drains when you are sprinting. If stamina is depleted, you will move at normal speed again. Stamina regenerates on its own. The slower you move, the faster it regenerates. Stamina regeneration rates are: 
running < walking < not moving.

The inventory system in not very fleshed out. Right now, all you can do is add an item and display items in your inventory. The functions for both of these actions are exposed to blueprints, where they must be bound to keys in order to use them.
