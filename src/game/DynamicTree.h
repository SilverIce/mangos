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

#pragma once

namespace G3D
{
    class Vector3;
}

using G3D::Vector3;
class ModelInstance_Overriden;

class DynamicMapTree
{
    struct DynTreeImpl& impl;
public:

    DynamicMapTree();
    ~DynamicMapTree();

    bool isInLineOfSight(const Vector3& pos1, const Vector3& pos2, uint32 phasemask) const;
    bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask) const;

    void insert(const ModelInstance_Overriden&);
    void remove(const ModelInstance_Overriden&);
    bool contains(const ModelInstance_Overriden&) const;
    int size() const;

    void balance();
    void update(uint32 diff);
};

