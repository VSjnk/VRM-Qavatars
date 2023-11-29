#include "main.hpp"

#include "custom-types/shared/coroutine.hpp"
#include "custom-types/shared/register.hpp"

#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/MainCamera.hpp"

#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/QualitySettings.hpp"

#include "AssetLib/shaders/shaderLoader.hpp"
#include "AssetLib/shaders/ShaderSO.hpp"
#include "AssetLib/modelImporter.hpp"

#include "UI/AvatarsFlowCoordinator.hpp"

#include "LightManager.hpp"
#include "SceneEventManager.hpp"
#include "GroundOffsetManager.hpp"
#include "MirrorManager.hpp"

#include "config/ConfigManager.hpp"

#include "VMC/VMCClient.hpp"
#include "VMC/VMCServer.hpp"

static ModInfo modInfo;

Logger& getLogger() {
    static Logger* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

#define coro(...) custom_types::Helpers::CoroutineHelper::New(__VA_ARGS__)

custom_types::Helpers::Coroutine Setup() {

    UnityEngine::GameObject::New_ctor("LightManager")->AddComponent<VRMQavatars::LightManager*>();

    getLogger().info("Starting Load!");

    UnityEngine::AssetBundle* ass;
    co_yield coro(VRM::ShaderLoader::LoadBundleFromFileAsync("sdcard/ModData/shaders.sbund", ass));
    if (!ass)
    {
        getLogger().error("Couldn't load bundle from file, dieing...");
        co_return;
    }

    VRMData::ShaderSO* data = nullptr;
    co_yield coro(VRM::ShaderLoader::LoadAssetFromBundleAsync(ass, "Assets/shaders.asset", csTypeOf(VRMData::ShaderSO*), reinterpret_cast<UnityEngine::Object*&>(data)));
    if(data == nullptr)
    {
        getLogger().error("Couldn't load asset...");
        co_return;
    }

    ass->Unload(false);

    AssetLib::ModelImporter::mtoon = data->mToonShader;
    VRMQavatars::MirrorManager::mirrorShader = data->mirrorShader;

    if(VRMQavatars::AvatarManager::currentContext == nullptr)
    {
        auto& globcon = VRMQavatars::Config::ConfigManager::GetGlobalConfig();
        if(globcon.hasSelected.GetValue())
        {
            const auto path = globcon.selectedFileName.GetValue();
            getLogger().info("%s", (std::string(vrm_path) + "/" + path).c_str());
            if(fileexists(std::string(vrm_path) + "/" + path))
            {
                const auto ctx = AssetLib::ModelImporter::LoadVRM(std::string(vrm_path) + "/" + path, AssetLib::ModelImporter::mtoon.ptr());
                VRMQavatars::AvatarManager::SetContext(ctx);

                auto& avaConfig = VRMQavatars::Config::ConfigManager::GetAvatarConfig();
                if(avaConfig.HasCalibrated.GetValue())
                {
                    VRMQavatars::AvatarManager::CalibrateScale(avaConfig.CalibratedScale.GetValue());
                }
            }
        }
    }

    VRMQavatars::VMC::VMCClient::InitClient();
    VRMQavatars::VMC::VMCServer::InitServer();

    VRMQavatars::GroundOffsetManager::Init();

    VRMQavatars::MirrorManager::CreateMainMirror();

    static auto shadows = il2cpp_utils::resolve_icall<void, int>("UnityEngine.QualitySettings::set_shadows");
    shadows(2); //all

    static auto shadowResolution = il2cpp_utils::resolve_icall<void, int>("UnityEngine.QualitySettings::set_shadowResolution");
    shadowResolution(3); //very high

    static auto shadowCascades = il2cpp_utils::resolve_icall<void, int>("UnityEngine.QualitySettings::set_shadowCascades");
    shadowCascades(4);

    auto mat = data->shadowMaterial;

    auto ground = UnityEngine::GameObject::Find("BasicMenuGround");

    auto renderer = ground->GetComponent<UnityEngine::MeshRenderer*>();
    auto oldMat = renderer->get_material();

    renderer->set_materials({oldMat, mat});

    co_return;
}
 
MAKE_HOOK_MATCH(MainMenuUIHook, &GlobalNamespace::MainMenuViewController::DidActivate, void, GlobalNamespace::MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainMenuUIHook(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if(firstActivation)
    {
        self->StartCoroutine(coro(Setup()));
    }
}

// FP/TP stuff should be in its own mod...
MAKE_HOOK_MATCH(MainCameraHook, &GlobalNamespace::MainCamera::Awake, void, GlobalNamespace::MainCamera* self)
{
    MainCameraHook(self);
    constexpr int fpmask =
           2147483647 &
           ~(1 << 3);
    self->get_camera()->set_cullingMask(fpmask);
}

//I know
//I know
//I know
//Scene manager is bad
//Open a PR (please)
MAKE_HOOK_MATCH(SceneManager_Internal_ActiveSceneChanged, &UnityEngine::SceneManagement::SceneManager::Internal_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene prevScene, UnityEngine::SceneManagement::Scene nextScene) {
    SceneManager_Internal_ActiveSceneChanged(prevScene, nextScene);
    VRMQavatars::SceneEventManager::GameSceneChanged(nextScene);
}

extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;

    getGlobalConfig().Init(Configuration::getConfigFilePath(modInfo));
}

extern "C" void load() {
    il2cpp_functions::Init();

    mkpath(vrm_path);
    mkpath(avaconfig_path);

    custom_types::Register::AutoRegister(); 

    BSML::Register::RegisterMainMenu<FlowCoordinators::AvatarsFlowCoordinator*>("Avatars", "VRM Custom Avatars");

    INSTALL_HOOK(getLogger(), MainMenuUIHook);
    INSTALL_HOOK(getLogger(), MainCameraHook);
    INSTALL_HOOK(getLogger(), SceneManager_Internal_ActiveSceneChanged);
}