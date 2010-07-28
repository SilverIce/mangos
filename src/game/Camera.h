
#ifndef MANGOSSERVER_CAMERA_H
#define MANGOSSERVER_CAMERA_H

#include "GridDefines.h"
#include "Dynamic/Signals.h"

class ViewPoint;
class WorldObject;
class UpdateData;
class WorldPacket;
class Player;

struct WorldObjectEvents
{
    MAKE_EVENT(WorldObjectEvents, on_added_to_world, void);
    MAKE_EVENT(WorldObjectEvents, on_removed_from_world, void);

    MAKE_EVENT(WorldObjectEvents, on_visibility_changed, void);
    MAKE_EVENT(WorldObjectEvents, on_moved, void);
};

/// Camera - object-receiver. Receives broadcast packets from nearby worldobjects, object visibility changes and sends them to client
class MANGOS_DLL_SPEC Camera : public IRecvr<WorldObjectEvents>
{
    friend class ViewPoint;
    public:

        explicit Camera(Player* pl);
        ~Camera();

        WorldObject* GetBody() { return m_source;}
        Player* GetOwner() { return &m_owner;}

        // set camera's view to any worldobject
        // Note: this worldobject must be in same map, in same phase with camera's owner(player)
        // client supports only unit and dynamic objects as farsight objects
        void SetView(WorldObject *obj);

        // set view to camera's owner
        void ResetView();

        template<class T>
        void UpdateVisibilityOf(T * obj, UpdateData &d, std::set<WorldObject*>& vis);
        void UpdateVisibilityOf(WorldObject* obj);

        void ReceivePacket(WorldPacket *data);

        // updates visibility of worldobjects around viewpoint for camera's owner
        void UpdateVisibilityForOwner();

    private:
        // called when viewpoint changes visibility state
        void on_added_to_world(void);
        void on_removed_from_world(void);

        void on_visibility_changed(void);
        void on_moved(void);

        Player& m_owner;
        WorldObject* m_source;

        void UpdateForCurrentViewPoint();

    public:
        GridReference<Camera>& GetGridRef() { return m_gridRef; }
        bool isActiveObject() const { return false; }
    private:
        template<class A, class T, class Y> friend class Grid;
        void SetGrid(GridType * g) { m_currGrid = g; }

        GridType * m_currGrid;
        GridReference<Camera> m_gridRef;
};

/// Object-observer, notifies farsight object state to cameras that attached to it
class MANGOS_DLL_SPEC ViewPoint : public Observer<WorldObjectEvents>
{
    friend class Camera;
    typedef Observer<WorldObjectEvents> base;

    uint32 cameras_count;

    void Attach(Camera* c)
    {
        c->Observe(this);
        ++cameras_count;
    }

    void Detach(Camera* c)
    {
        c->Detach();
        --cameras_count;
    }

public:

    ViewPoint( ) : cameras_count(0) {}
    ~ViewPoint();

    bool hasViewers() const { return cameras_count != 0; }
};

#endif
