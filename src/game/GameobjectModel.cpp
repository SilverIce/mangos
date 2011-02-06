/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "vmap/VMapFactory.h"
#include "vmap/VMapManager2.h"
#include "vmap/VMapDefinitions.h"

#include "GameObject.h"
#include "World.h"
#include "GameobjectModel.h"

typedef UNORDERED_MAP<uint32, std::string> ModelList;
ModelList model_list;

void LoadGameObjectModelList()
{
    FILE * model_list_file = fopen((sWorld.GetDataPath() + "vmaps/" + VMAP::GAMEOBJECT_MODELS).c_str(), "rb");
    if (!model_list_file)
        return;

    uint32 name_length, displayId;
    char buff[500];
    while (!feof(model_list_file))
    {
        fread(&displayId,sizeof(uint32),1,model_list_file);
        fread(&name_length,sizeof(uint32),1,model_list_file);

        if (name_length >= sizeof(buff))
        {
            std::cout << "\nFile '" << VMAP::GAMEOBJECT_MODELS << "' seems to be corrupted" << std::endl;
            break;
        }

        fread(&buff,sizeof(char),name_length,model_list_file);

        model_list.insert
        (
            ModelList::value_type(displayId, std::string(buff,name_length))
        );
    }
    fclose(model_list_file);
}

ModelInstance_Overriden::~ModelInstance_Overriden()
{
    if (iModel)
        ((VMAP::VMapManager2*)VMAP::VMapFactory::createOrGetVMapManager())->releaseModelInstance(name);
}

bool ModelInstance_Overriden::initialize(const GameObject & go, const GameObjectDisplayInfoEntry& info)
{
    G3D::AABox mdl_box((Vector3&)info.bound[0], (Vector3&)info.bound[3]);
    // ignore models with no bounds
    if (mdl_box == G3D::AABox::zero())
        return false;

    ModelList::const_iterator it = model_list.find(info.Displayid);
    if (it == model_list.end())
        return false;

    iModel = ((VMAP::VMapManager2*)VMAP::VMapFactory::createOrGetVMapManager())->acquireModelInstance(sWorld.GetDataPath() + "vmaps/", it->second);

    if (!iModel)
        return false;

    flags = VMAP::MOD_M2;
    adtId = 0;
    ID = 0;
    iPos = Vector3(go.GetPositionX(),go.GetPositionY(),go.GetPositionZ());
    iRot = Vector3(0, go.GetOrientation()*180.f/G3D::pi(), 0);
    iScale = go.GetObjectScale();
    iInvScale = 1.f/iScale;
    name = it->second;

    G3D::Matrix3 iRotation = G3D::Matrix3::fromEulerAnglesZYX(G3D::pi()*iRot.y/180.f, G3D::pi()*iRot.x/180.f, G3D::pi()*iRot.z/180.f);
    iInvRot = iRotation.inverse();

    // transform bounding box:
    mdl_box = G3D::AABox(mdl_box.low() * iScale, mdl_box.high() * iScale);

    G3D::AABox rotated_bounds;
    for (int i = 0; i < 8; ++i)
        rotated_bounds.merge(iRotation * mdl_box.corner(i));

    this->iBound = rotated_bounds + iPos;

    return true;
}

ModelInstance_Overriden* ModelInstance_Overriden::construct(const class GameObject & go, const struct GameObjectDisplayInfoEntry& info)
{
    ModelInstance_Overriden* mdl = new ModelInstance_Overriden();
    if (!mdl->initialize(go, info))
    {
        delete mdl;
        return NULL;
    }

    return mdl;
}