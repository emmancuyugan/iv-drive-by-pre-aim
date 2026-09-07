#include <plugin.h>

#include "game_IV/CPed.h"

using namespace plugin;

volatile uint32_t g_value38 = 0;
volatile uint32_t g_value3C = 0;
volatile uint32_t g_value40 = 0;
volatile uint32_t g_value44 = 0;
volatile uint32_t g_value48 = 0;
volatile uint32_t g_value4C = 0;

struct Main
{
    Main()
    {
        Events::gameProcessEvent += [] {
            gInstance.OnGameProcess();
            };
    }

    void OnGameProcess()
    {
        CPed* player = FindPlayerPed(0);

        if (!player)
            return;

        if (!player->m_pVehicle)
            return;

        if (!player->m_pPedIntelligence)
            return;

        uint8_t* intelligence =
            reinterpret_cast<uint8_t*>(player->m_pPedIntelligence);

        g_value38 = *reinterpret_cast<uint32_t*>(intelligence + 0x38);
        g_value3C = *reinterpret_cast<uint32_t*>(intelligence + 0x3C);
        g_value40 = *reinterpret_cast<uint32_t*>(intelligence + 0x40);
        g_value44 = *reinterpret_cast<uint32_t*>(intelligence + 0x44);
        g_value48 = *reinterpret_cast<uint32_t*>(intelligence + 0x48);
        g_value4C = *reinterpret_cast<uint32_t*>(intelligence + 0x4C);
    }
} gInstance;