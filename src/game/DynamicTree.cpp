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
#include "Timer.h"
#include "GameobjectModel.h"
#include "Log.h"

using G3D::Ray;

template<> struct HashTrait<const  ModelInstance_Overriden*>{
    static size_t hashCode(const ModelInstance_Overriden* g) { return (size_t)(void*)g; }
};

template<> struct PositionTrait<const  ModelInstance_Overriden*> {
    static void getPosition(const ModelInstance_Overriden* g, Vector3& p) { p = g->getPosition(); }
};

template<> struct BoundsTrait<const  ModelInstance_Overriden*> {
    static void getBounds(const ModelInstance_Overriden* g, G3D::AABox& out) { out = g->getBounds();}
};

#define log(str, ...) printf((str"\n"),__VA_ARGS__)

int valuesPerNode = 5, numMeanSplits = 3;

int UNBALANCED_TIMES_LIMIT = 20;
int CHECK_TREE_PERIOD = 20000;

struct KDTreeTest : public G3D::KDTree2<const ModelInstance_Overriden*>
{
    typedef G3D::KDTree2<const ModelInstance_Overriden*> base;

    KDTreeTest() :
        rebalance_timer(CHECK_TREE_PERIOD),
        unbalanced_times(0)
    {
        MANGOS_ASSERT(!root && "root shouln't be created");
        root = new Node();
    }

    void insert(const ModelInstance_Overriden& mdl)
    {
        base::insert(&mdl);
        ++unbalanced_times;
    }

    void remove(const ModelInstance_Overriden& mdl)
    {
        base::remove(&mdl);
        ++unbalanced_times;
    }

    void clear()
    {
        base::clear();
        unbalanced_times = 0;
    }

    void balance()
    {
        base::balance(valuesPerNode, numMeanSplits);
        sLog.outString("DynamicMapTree balanced, total objects count: %u, unbalanced %u",size(),unbalanced_times);
        unbalanced_times = 0;
    }

    void update(uint32 t_diff)
    {
        if (!size())
            return;

        rebalance_timer.Update(t_diff);
        if (rebalance_timer.Passed())
        {
            rebalance_timer.Reset(CHECK_TREE_PERIOD);
            if (unbalanced_times > UNBALANCED_TIMES_LIMIT)
                balance();
        }
    }

    ShortTimeTracker rebalance_timer;
    int unbalanced_times;
};

DynamicMapTree::DynamicMapTree() : impl(*new KDTreeTest())
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
    return impl.contains(&mdl);
}

void DynamicMapTree::clear()
{
    impl.clear();
}

void DynamicMapTree::balance(int  /*= 5*/, int  /*= 3*/)
{
    impl.balance();
}

struct DynamicTreeIntersectionCallback
{
    bool did_hit;
    uint32 phase_mask;
    DynamicTreeIntersectionCallback(uint32 phasemask) : did_hit(false), phase_mask(phasemask) {}
    bool operator()(const Ray& r, const ModelInstance_Overriden* obj, float& distance)
    {
        bool hit = obj->intersectRay(r, distance, true, phase_mask);
        if (hit)
            did_hit = true;
        return hit;
    }
};

struct DynamicTreeIntersectionCallback_WithLogger
{
    bool did_hit;
    uint32 phase_mask;
    DynamicTreeIntersectionCallback_WithLogger(uint32 phasemask) : did_hit(false), phase_mask(phasemask)
    {
        log("Dynamic Intersection log");
    }
    bool operator()(const Ray& r, const ModelInstance_Overriden* obj, float& distance)
    {
        log("testing intersection with %s", obj->name.c_str());
        bool hit = obj->intersectRay(r, distance, true, phase_mask);
        if (hit)
        {
            did_hit = true;
            log("result: intersects");
        }
        return hit;
    }
};

bool DynamicMapTree::isInLineOfSight(const Vector3& v1, const Vector3& v2, uint32 phasemask) const
{
    float maxDist = (v2 - v1).magnitude();

    if (!G3D::fuzzyGt(maxDist, 0) )
        return true;

    Ray r = Ray::fromOriginAndDirection(v1, (v2-v1) / maxDist);
    DynamicTreeIntersectionCallback callback(phasemask);
    impl.intersectRay(r, callback, maxDist, true, true);

    return !callback.did_hit;
}

bool DynamicMapTree::isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask) const
{
    return isInLineOfSight(Vector3(x1,y1,z1), Vector3(x2,y2,z2), phasemask);
}

int DynamicMapTree::size() const
{
    return impl.size();
}

void DynamicMapTree::update(uint32 t_diff)
{
    impl.update(t_diff);
}
