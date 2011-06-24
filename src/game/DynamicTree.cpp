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

#include "DynamicTree.h"
#include "KDTree2.h"
//#include "QuadTree.h"
#include "RegularGrid.h"
#include "BIHWrap.h"

#include "Timer.h"
#include "GameobjectModel.h"
#include "Log.h"
#include "ModelInstance.h"

using VMAP::ModelInstance;
using G3D::Ray;
using G3D::KDTree2;

template<> struct HashTrait< ModelInstance_Overriden>{
    static size_t hashCode(const ModelInstance_Overriden& g) { return (size_t)(void*)&g; }
};

template<> struct PositionTrait< ModelInstance_Overriden> {
    static void getPosition(const ModelInstance_Overriden& g, Vector3& p) { p = g.getPosition(); }
};

template<> struct BoundsTrait< ModelInstance_Overriden> {
    static void getBounds(const ModelInstance_Overriden& g, G3D::AABox& out) { out = g.getBounds();}
    static void getBounds2(const ModelInstance_Overriden* g, G3D::AABox& out) { out = g->getBounds();}
};

static bool operator == (const ModelInstance_Overriden& mdl, const ModelInstance_Overriden& mdl2){
    return &mdl == &mdl2;
}


#define log(str, ...) printf((str"\n"),__VA_ARGS__)

int valuesPerNode = 5, numMeanSplits = 3;

int UNBALANCED_TIMES_LIMIT = 5;
int CHECK_TREE_PERIOD = 20000;

typedef RegularGrid2D<ModelInstance_Overriden, BIHWrap<ModelInstance_Overriden> > ParentTree;

struct DynTreeImpl : public ParentTree/*, public Intersectable*/
{
    typedef ModelInstance_Overriden Model;
    typedef ParentTree base;

    DynTreeImpl() :
        rebalance_timer(CHECK_TREE_PERIOD),
        unbalanced_times(0)
    {
    }

    void insert(const Model& mdl)
    {
        base::insert(mdl);
        ++unbalanced_times;
    }

    void remove(const Model& mdl)
    {
        base::remove(mdl);
        ++unbalanced_times;
    }

    void balance()
    {
        base::balance();
        unbalanced_times = 0;
    }

    void update(uint32 difftime)
    {
        if (!size())
            return;

        rebalance_timer.Update(difftime);
        if (rebalance_timer.Passed())
        {
            rebalance_timer.Reset(CHECK_TREE_PERIOD);
            if (unbalanced_times > 0)
                balance();
        }
    }

    ShortTimeTracker rebalance_timer;
    int unbalanced_times;
};

#pragma region delegates
DynamicMapTree::DynamicMapTree() : impl(*new DynTreeImpl())
{
}

DynamicMapTree::~DynamicMapTree()
{
    delete &impl;
}

void DynamicMapTree::insert(const ModelInstance_Overriden& mdl)
{
    impl.insert(mdl);
}

void DynamicMapTree::remove(const ModelInstance_Overriden& mdl)
{
    impl.remove(mdl);
}

bool DynamicMapTree::contains(const ModelInstance_Overriden& mdl) const
{
    return impl.contains(mdl);
}

void DynamicMapTree::balance()
{
    impl.balance();
}

int DynamicMapTree::size() const
{
    return impl.size();
}

void DynamicMapTree::update(uint32 t_diff)
{
    impl.update(t_diff);
}
#pragma endregion

struct DynamicTreeIntersectionCallback
{
    bool did_hit;
    uint32 phase_mask;
    DynamicTreeIntersectionCallback(uint32 phasemask) : did_hit(false), phase_mask(phasemask) {}
    bool operator()(const Ray& r, const ModelInstance_Overriden& obj, float& distance)
    {
        did_hit = obj.intersectRay(r, distance, true, phase_mask);
        return did_hit;
    }
    bool didHit() const { return did_hit;}
};

struct DynamicTreeIntersectionCallback_WithLogger
{
    bool did_hit;
    uint32 phase_mask;
    DynamicTreeIntersectionCallback_WithLogger(uint32 phasemask) : did_hit(false), phase_mask(phasemask)
    {
        log("Dynamic Intersection log");
    }
    bool operator()(const Ray& r, const ModelInstance_Overriden& obj, float& distance)
    {
        log("testing intersection with %s", obj.name.c_str());
        bool hit = obj.intersectRay(r, distance, true, phase_mask);
        if (hit)
        {
            did_hit = true;
            log("result: intersects");
        }
        return hit;
    }
    bool didHit() const { return did_hit;}
};

bool DynamicMapTree::isInLineOfSight(const Vector3& v1, const Vector3& v2, uint32 phasemask) const
{
    float maxDist = (v2 - v1).magnitude();

    if (!G3D::fuzzyGt(maxDist, 0) )
        return true;

    Ray r(v1, (v2-v1) / maxDist);
    DynamicTreeIntersectionCallback callback(phasemask);
    impl.intersectRay(r, callback, maxDist);

    return !callback.did_hit;
}

bool DynamicMapTree::isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask) const
{
    return isInLineOfSight(Vector3(x1,y1,z1), Vector3(x2,y2,z2), phasemask);
}
