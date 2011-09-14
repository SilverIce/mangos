#include "WorldSocket.h"
#include "WorldPacket.h"
#include "G3D\BinaryOutput.h"
#include "G3D\BinaryInput.h"
#include "G3D\NetworkDevice.h"
#include "G3D\GThread.h"
#include "G3D\Log.h"

enum PlayerPacketOpcodes
{
    MSG_NONE = 0,
    CMSG_SEND_PACKET = 1,
    MSG_HELLO = 2,
    CMSG_MOVER_GUID = 3,
    SMSG_MOVER_GUID = 4,
};

class SniffPacket : public WorldPacket
{
public:

    void serialize(G3D::BinaryOutput& out) const;

    void deserialize(G3D::BinaryInput& in)
    {
        this->SetOpcode(in.readUInt16());
        int64 size = in.getLength() - 2;
        if (size > 1)
        {
            this->resize(size);
            in.readBytes((void*)this->contents(), size);
        }
        else
            this->clear();
    }
};

WorldSocket * g_socket = NULL;

namespace G3D
{
    void SniffPlayerLoop(void*);

    static GThreadRef thr = GThread::create("sniff_player", &SniffPlayerLoop);
    static volatile bool running = false;

    void SniffPlayerLoop(void*)
    {
        NetAddress adress("127.0.0.1", 45686);

        NetListenerRef list = NetListener::create(45686);
        ReliableConduitRef cnd;// = list->waitForConnection();

        SniffPacket packet;
        while(running)
        {
            if (cnd.isNull() || !cnd->ok())
            {
                cnd = list->waitForConnection();
                if (cnd.isNull())
                {
                    Log::common()->printf("NetListener::waitForConnection returned null");
                    return;
                }
            }

            PlayerPacketOpcodes msg(MSG_NONE);
            while ((msg = (PlayerPacketOpcodes)cnd->waitingMessageType())
                && cnd->receive(packet))
            {
                if (g_socket && msg == CMSG_SEND_PACKET)
                    g_socket->SendPacket(packet);
            }

            System::sleep(20/1000.f);
        }
    }

    void RunSniffPlayer(bool run)
    {
        if (run == running)
            return;

        running = run;
        if (run)
            thr->start();
        else
            thr->waitForCompletion();
    }
}

void RunSniffPlayer(bool run)
{
    G3D::RunSniffPlayer(run);
}
