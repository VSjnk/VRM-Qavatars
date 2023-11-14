#pragma once

#include "customTypes/WristTwistFix.hpp"

#include "main.hpp"
#include "AssetLib/structure/VRM/VRMmodelContext.hpp"

#include "customTypes/TargetManager.hpp"

#include "utils/structs/OffsetPose.hpp"

#include "RootMotion/FinalIK/VRIK.hpp"
#include "RootMotion/FinalIK/IKSolverVR.hpp"
#include "RootMotion/FinalIK/IKSolverVR_Arm.hpp"
#include "RootMotion/FinalIK/IKSolverVR_Leg.hpp"
#include "RootMotion/FinalIK/IKSolverVR_Locomotion.hpp"
namespace VRMQavatars {
    class AvatarManager
    {
        public:
        static AssetLib::Structure::VRM::VRMModelContext* currentContext;
        static void SetContext(AssetLib::Structure::VRM::VRMModelContext* context);
        static void SetLegSwivel(float value);
        static void SetArmSwivel(float value);
        static void SetHandOffset(const Structs::OffsetPose& pose);
        static void SetBodyStiffness(float value);
        static void SetShoulderRotation(float value);
        static void SetWristFixWeight(float value);
        static void SetShoulderFixWeight(float value);
        private:
        static RootMotion::FinalIK::VRIK* _vrik;
        static TargetManager* _targetManager;
        static WristTwistFix* _wristTwistFix;
    };
};