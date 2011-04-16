//#include "pchdef.h"

//add here most rarely modified headers to speed up debug build compilation
/*
#include "WorldSocket.h"                                    // must be first to make ACE happy with ACE includes in it
#include "Common.h"

#include "MapManager.h" -->
    #include "Common.h"
    #include "Platform/Define.h"
    #include "Policies/Singleton.h"
    #include "ace/Recursive_Thread_Mutex.h"
    #include "Map.h"
    #include "GridStates.h"

#include "Log.h"
#include "ObjectAccessor.h" -->
    #include "Common.h"
    #include "Platform/Define.h"
    #include "Policies/Singleton.h"
    #include <ace/Thread_Mutex.h>
    #include <ace/RW_Thread_Mutex.h>
    #include "Utilities/UnorderedMapSet.h"
    #include "Policies/ThreadingModel.h"
    #include "UpdateData.h"
    #include "GridDefines.h"
    #include "Object.h"
    #include "Player.h"
    #include "Corpse.h"

#include "ObjectGuid.h"
#include "SQLStorages.h"
#include "Opcodes.h"
#include "SharedDefines.h"

#include "ObjectMgr.h" -->
    #include "Common.h"
    #include "Log.h"
    #include "Object.h"
    #include "Bag.h"
    #include "Creature.h"
    #include "Player.h"
    #include "GameObject.h"
    #include "Corpse.h"
    #include "QuestDef.h"
    #include "ItemPrototype.h"
    #include "NPCHandler.h"
    #include "Database/DatabaseEnv.h"
    #include "Map.h"
    #include "MapPersistentStateMgr.h"
    #include "ObjectAccessor.h"
    #include "ObjectGuid.h"
    #include "Policies/Singleton.h"
    #include "SQLStorages.h"
    #include <string>
    #include <map>
    #include <limits>


#include "ScriptMgr.h"
*/

/*
#include "Common.h"

#include "WorldPacket.h"
#include "World.h"
#include "Util.h"
#include "Policies/Singleton.h"

#include "Opcodes.h"
#include "SharedDefines.h"
#include "Language.h"
#include "QuestDef.h"
#include "ItemPrototype.h"

#include "CellImpl.h"
#include "MapManager.h"
#include "CellImpl.h"

#include "SpellMgr.h"
#include "Spell.h"
#include "SpellAuras.h"

#include "Object.h"
#include "Bag.h"
#include "Corpse.h"
#include "Totem.h"
#include "Player.h"
#include "DynamicObject.h"
#include "GameObject.h"
#include "CreatureAI.h"*/