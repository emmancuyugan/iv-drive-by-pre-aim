#include "plugin.h"
#include "CPad.h"
#include "safetyhook/safetyhook.hpp"

using namespace plugin;

class GTAIVDriveByPreAim {
public:
    static inline SafetyHookInline shPlayerWantsToDoDriveby;

    static char __cdecl PlayerWantsToDoDriveby(int a1) {

        char result =
            shPlayerWantsToDoDriveby.unsafe_ccall<char>(a1);
        
        if (CPad::IsMouseButtonPressed(2))
            result = 1;

        return result;
    }

    GTAIVDriveByPreAim() {
        static auto pattern =
            plugin::GetGlobalAddress(
                plugin::pattern::Get(
                    "51 57 8B 7C 24 ? 80 BF ? ? ? ? ? 75",
                    0
                )
            );

        shPlayerWantsToDoDriveby =
            safetyhook::create_inline(
                pattern,
                PlayerWantsToDoDriveby
            );
    }
};

GTAIVDriveByPreAim gtaIVDriveByPreAim;
