#include "plugin.h"
#include "CPad.h"
#include "injector/injector.hpp"
#include "safetyhook/safetyhook.hpp"

using namespace plugin;

class GTAIVDriveByPreAim {
public:

    // DRIVE-BY

    static inline SafetyHookInline shPlayerWantsToDoDriveby;

    static char __cdecl PlayerWantsToDoDriveby(int a1) {
        char result =
            shPlayerWantsToDoDriveby.unsafe_ccall<char>(a1);

        if (CPad::IsMouseButtonPressed(2))
            result = 1;

        return result;
    }

    // CAMERA HOOKS

    static inline injector::hook_back<
        void(__fastcall*)(rage::Matrix44*, void*, void*)
    > hbCopyMatFront;

    static inline injector::hook_back<
        void(__fastcall*)(rage::Matrix44*, void*, void*)
    > hbCopyMatBehind;

    // CAMERA STATE

    static inline float cameraZoom = 0.0f;
    static inline float cameraHorizontal = 0.0f;
    static inline float cameraVertical = 0.0f;

    // CAMERA SETTINGS

    // Existing zoom that already looked good.
    static constexpr float CAMERA_ZOOM_AMOUNT = 1.5f;

    // GTA V-style upper-left framing.
    static constexpr float CAMERA_HORIZONTAL_AMOUNT = -0.45f;
    static constexpr float CAMERA_VERTICAL_AMOUNT = 0.30f;

    // Smooth transition.
    static constexpr float CAMERA_ZOOM_SPEED = 0.15f;
    static constexpr float CAMERA_OFFSET_SPEED = 0.15f;

    // UPDATE CAMERA

    static void UpdateCameraZoom() {

        const bool aiming =
            CPad::IsMouseButtonPressed(2);

        const float zoomTarget =
            aiming ? CAMERA_ZOOM_AMOUNT : 0.0f;

        const float horizontalTarget =
            aiming ? CAMERA_HORIZONTAL_AMOUNT : 0.0f;

        const float verticalTarget =
            aiming ? CAMERA_VERTICAL_AMOUNT : 0.0f;


        // Zoom
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


        // Horizontal framing.
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


        // Vertical framing.
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

    // APPLY CAMERA OFFSET

    static void ApplyDriveByCamera(
        rage::Matrix44* mat
    ) {
        if (cameraZoom > 0.0f) {
            mat->pos += mat->up * cameraZoom;
        }

        if (cameraHorizontal != 0.0f) {
            mat->pos += mat->right * cameraHorizontal;
        }

        if (cameraVertical != 0.0f) {
            mat->pos += mat->up * cameraVertical;
        }
    }

    // FRONT CAMERA

    static void __fastcall CopyMatFront(
        rage::Matrix44* mat,
        void*,
        void* arg2
    ) {
        hbCopyMatFront.fun(mat, 0, arg2);

        ApplyDriveByCamera(mat);
    }

    // BEHIND CAMERA

    static void __fastcall CopyMatBehind(
        rage::Matrix44* mat,
        void*,
        void* arg2
    ) {
        hbCopyMatBehind.fun(mat, 0, arg2);

        ApplyDriveByCamera(mat);
    }

    // CONSTRUCTOR

    GTAIVDriveByPreAim() {

        // DRIVE-BY

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

        // FRONT CAMERA

        static auto frontPattern =
            plugin::GetGlobalAddress(
                plugin::pattern::Get(
                    "E8 ? ? ? ? 80 A7 ? ? ? ? ? 80 A7 ? ? ? ? ? 80 7C 24",
                    0
                )
            );

        hbCopyMatFront.fun =
            injector::GetBranchDestination(
                frontPattern
            ).get();

        injector::MakeCALL(
            frontPattern,
            CopyMatFront
        );

        // BEHIND CAMERA

        static auto behindPattern =
            plugin::GetGlobalAddress(
                plugin::pattern::Get(
                    "E8 ? ? ? ? 5F B0 01 5E 8B E5 5D C2 14 00",
                    0
                )
            );

        hbCopyMatBehind.fun =
            injector::GetBranchDestination(
                behindPattern
            ).get();

        injector::MakeCALL(
            behindPattern,
            CopyMatBehind
        );

        // CAMERA UPDATE

        plugin::Events::gameProcessEvent += [] {
            UpdateCameraZoom();
            };
    }
};

GTAIVDriveByPreAim gtaIVDriveByPreAim;
