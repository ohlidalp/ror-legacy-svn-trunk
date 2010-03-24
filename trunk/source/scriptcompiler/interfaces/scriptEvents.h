enum scriptEvents
{
	SE_COLLISION_BOX_ENTER             = 0x00000001, //!< triggered when truck or person enters a previous registered collision box, the argument refers to the collision box ID
	SE_COLLISION_BOX_LEAVE             = 0x00000002, //!< triggered when truck or person leaves a previous registered and entered collision box, the argument refers to the collision box ID

	SE_TRUCK_ENTER                     = 0x00000004, //!< triggered when switching from person mode to truck mode, the argument refers to the truck number
	SE_TRUCK_EXIT                      = 0x00000008, //!< triggered when switching from truck mode to person mode, the argument refers to the truck number

	SE_TRUCK_ENGINE_DIED               = 0x00000010, //!< triggered when the trucks engine dies (from underrev, water, etc), the argument refers to the truck number
	SE_TRUCK_ENGINE_FIRE               = 0x00000020, //!< triggered when the planes engines start to get on fire, the argument refers to the truck number
	SE_TRUCK_TOUCHED_WATER             = 0x00000040, //!< triggered when any part of the truck touches water, the argument refers to the truck number
	SE_TRUCK_BEAM_BROKE                = 0x00000080, //!< triggered when a beam breaks, the argument refers to the truck number
	SE_TRUCK_LOCKED                    = 0x00000100, //!< triggered when the truck got lock to another truck, the argument refers to the truck number
	SE_TRUCK_UNLOCKED                  = 0x00000200, //!< triggered when the truck unlocks again, the argument refers to the truck number
	SE_TRUCK_LIGHT_TOGGLE              = 0x00000400, //!< triggered when the main light is toggled, the argument refers to the truck number
	SE_TRUCK_SKELETON_TOGGLE           = 0x00000800, //!< triggered when the user enters or exits skeleton mode, the argument refers to the truck number
	SE_TRUCK_TIE_TOGGLE                = 0x00001000, //!< triggered when the user toggles ties, the argument refers to the truck number
	SE_TRUCK_PARKINGBREAK_TOGGLE       = 0x00002000, //!< triggered when the user toggles the parking break, the argument refers to the truck number
	SE_TRUCK_BEACONS_TOGGLE            = 0x00004000, //!< triggered when the user toggles beacons, the argument refers to the truck number
	SE_TRUCK_CPARTICLES_TOGGLE         = 0x00008000, //!< triggered when the user toggles custom particles, the argument refers to the truck number
	SE_TRUCK_GROUND_CONTACT_CHANGED    = 0x00010000, //!< triggered when the trucks ground contact changed (no contact, different ground models, etc), the argument refers to the truck number

	SE_GENERIC_NEW_TRUCK               = 0x00020000, //!< triggered when the user spawns a new truck, the argument refers to the truck number
	SE_GENERIC_DELETED_TRUCK           = 0x00040000, //!< triggered when the user deletes a truck, the argument refers to the truck number

	SE_GENERIC_INPUT_EVENT             = 0x00080000, //!< triggered when an input event bound to the scripting engine is toggled, the argument refers to event id
	SE_GENERIC_MOUSE_BEAM_INTERACTION  = 0x00100000, //!< triggered when the user uses the mouse to interact with the truck, the argument refers to the truck number

};