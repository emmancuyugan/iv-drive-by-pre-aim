#include "plugin.h"
#include "CPad.h"
#include "injector/injector.hpp"
#include "safetyhook/safetyhook.hpp"

using namespace plugin;

class GTAIVDriveByPreAim {
private:
    // GTA IV's cellphone display state, resolved from the same native
    // global used by FusionFix's cellphone drive-by fix.
    static inline bool* pPhoneDisplayMobile = nullptr;

    static void ResolvePhoneState() {
        if (pPhoneDisplayMobile != nullptr)
            return;

        // Complete Edition pattern.
        hook::pattern phonePattern(
            "C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 8B 01"
        );

        // Alternate pattern supported by FusionFix.
        if (phonePattern.empty()) {
            phonePattern = hook::pattern(
                "88 1D ? ? ? ? 88 1D ? ? ? ? 89 1D ? ? ? ? 8B 11"
            );
        }

        if (phonePattern.empty())
            return;

        pPhoneDisplayMobile =
            *phonePattern.get_first<bool*>(2);
    }

    static bool IsPhoneUp() {
        return pPhoneDisplayMobile != nullptr &&
            *pPhoneDisplayMobile;
    }

    // ---------------------------------------------------------------------
    // Drive-by
    // ---------------------------------------------------------------------

    static inline SafetyHookInline shPlayerWantsToDoDriveby;

    static char __cdecl PlayerWantsToDoDriveby(int a1) {
        // Prevent drive-by activation while the cellphone is displayed.
        if (IsPhoneUp())
            return 0;

        char result =
            shPlayerWantsToDoDriveby.unsafe_ccall<char>(a1);

        // RMB enables the drive-by preparation state.
        if (CPad::IsMouseButtonPressed(2))
            result = 1;

        return result;
    }

    // ---------------------------------------------------------------------
    // Camera
    // ---------------------------------------------------------------------

    static inline injector::hook_back<
        void(__fastcall*)(rage::Matrix44*, void*, void*)
    > hbCopyMatFront;

    static inline injector::hook_back<
        void(__fastcall*)(rage::Matrix44*, void*, void*)
    > hbCopyMatBehind;

    static inline float cameraZoom = 0.0f;
    static inline float cameraHorizontal = 0.0f;
    static inline float cameraVertical = 0.0f;

    static constexpr float CAMERA_ZOOM_AMOUNT = 1.5f;
    static constexpr float CAMERA_HORIZONTAL_AMOUNT = -0.45f;
    static constexpr float CAMERA_VERTICAL_AMOUNT = 0.30f;

    static constexpr float CAMERA_ZOOM_SPEED = 0.15f;
    static constexpr float CAMERA_OFFSET_SPEED = 0.15f;

    static void UpdateCameraZoom() {
        const bool phoneUp = IsPhoneUp();

        if (phoneUp) {
            cameraZoom = 0.0f;
            cameraHorizontal = 0.0f;
            cameraVertical = 0.0f;
            return;
        }

        const bool aiming =
            CPad::IsMouseButtonPressed(2);

        const float zoomTarget =
            aiming ? CAMERA_ZOOM_AMOUNT : 0.0f;

        const float horizontalTarget =
            aiming ? CAMERA_HORIZONTAL_AMOUNT : 0.0f;

        const float verticalTarget =
            aiming ? CAMERA_VERTICAL_AMOUNT : 0.0f;

        if (cameraZoom < zoomTarget) {
            cameraZoom += CAMERA_ZOOM_SPEED;

            if (cameraZoom > zoomTarget)
                cameraZoom = zoomTarget;
        }
        else if (cameraZoom > zoomTarget) {
            cameraZoom -= CAMERA_ZOOM_SPEED;

            if (cameraZoom < zoomTarget)
                cameraZoom = zoomTarget;
        }

        if (cameraHorizontal < horizontalTarget) {
            cameraHorizontal += CAMERA_OFFSET_SPEED;

            if (cameraHorizontal > horizontalTarget)
                cameraHorizontal = horizontalTarget;
        }
        else if (cameraHorizontal > horizontalTarget) {
            cameraHorizontal -= CAMERA_OFFSET_SPEED;

            if (cameraHorizontal < horizontalTarget)
                cameraHorizontal = horizontalTarget;
        }

        if (cameraVertical < verticalTarget) {
            cameraVertical += CAMERA_OFFSET_SPEED;

            if (cameraVertical > verticalTarget)
                cameraVertical = verticalTarget;
        }
        else if (cameraVertical > verticalTarget) {
            cameraVertical -= CAMERA_OFFSET_SPEED;

            if (cameraVertical < verticalTarget)
                cameraVertical = verticalTarget;
        }
    }

    static void ApplyDriveByCamera(rage::Matrix44* mat) {
        // The update normally clears the state when the phone opens.
        // Keep this guard here so the camera hook can never apply the
        // custom drive-by offset while the phone is displayed.
        if (IsPhoneUp())
            return;

        if (cameraZoom > 0.0f)
            mat->pos += mat->up * cameraZoom;

        if (cameraHorizontal != 0.0f)
            mat->pos += mat->right * cameraHorizontal;

        if (cameraVertical != 0.0f)
            mat->pos += mat->up * cameraVertical;
    }

    static void __fastcall CopyMatFront(
        rage::Matrix44* mat,
        void*,
        void* arg2
    ) {
        hbCopyMatFront.fun(mat, 0, arg2);
        ApplyDriveByCamera(mat);
    }

    static void __fastcall CopyMatBehind(
        rage::Matrix44* mat,
        void*,
        void* arg2
    ) {
        hbCopyMatBehind.fun(mat, 0, arg2);
        ApplyDriveByCamera(mat);
    }

public:
    GTAIVDriveByPreAim() {
        ResolvePhoneState();

        // Drive-by decision.
        static auto driveByPattern =
            plugin::GetGlobalAddress(
                plugin::pattern::Get(
                    "51 57 8B 7C 24 ? 80 BF ? ? ? ? ? 75",
                    0
                )
            );

        shPlayerWantsToDoDriveby =
            safetyhook::create_inline(
                driveByPattern,
                PlayerWantsToDoDriveby
            );

        // Front camera matrix.
        static auto frontPattern =
            plugin::GetGlobalAddress(
                plugin::pattern::Get(
                    "E8 ? ? ? ? 80 A7 ? ? ? ? ? 80 A7 ? ? ? ? ? 80 7C 24",
                    0
                )
            );

        hbCopyMatFront.fun =
            injector::GetBranchDestination(frontPattern).get();

        injector::MakeCALL(
            frontPattern,
            CopyMatFront
        );

        // Behind camera matrix.
        static auto behindPattern =
            plugin::GetGlobalAddress(
                plugin::pattern::Get(
                    "E8 ? ? ? ? 5F B0 01 5E 8B E5 5D C2 14 00",
                    0
                )
            );

        hbCopyMatBehind.fun =
            injector::GetBranchDestination(behindPattern).get();

        injector::MakeCALL(
            behindPattern,
            CopyMatBehind
        );

        // Update camera state once per game process tick.
        plugin::Events::gameProcessEvent += [] {
            UpdateCameraZoom();
            };
    }
};

GTAIVDriveByPreAim gtaIVDriveByPreAim;
