#include "imgui_setup.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS

#include "cimgui/cimgui.h"
#include "cimgui/cimgui_impl.h"

#include "user.h"
#include "resource_loader.h"

void imgui_init(tenv* env) {
  tuser_data* usr = env->usr;

  // Initialize resource manager (loads PAK if available)
  resource_manager_init();
  
  // On first run, extract resources from embedded PAK to AppData
  // This makes it a single-exe installer that auto-sets up resources
  printf("[ImGui] Ensuring resources extracted to AppData...\n");
  bool appdata_ready = resource_ensure_appdata_extracted();
  if (appdata_ready) {
    printf("[ImGui] Resources are ready in AppData\n");
  } else {
    printf("[ImGui] Warning: Could not extract to AppData, will use embedded/fallback\n");
  }

  igCreateContext(NULL);
  igImplVulkan_Init(&(ImGui_ImplVulkan_InitInfo){
      .ApiVersion = VK_API_VERSION_1_0,
      .Instance = env->ctx->instance,
      .PhysicalDevice = env->ctx->ph_device,
      .Device = env->ctx->device,
      .QueueFamily = env->ctx->queue_family,
      .Queue = env->ctx->queue,
      .DescriptorPool = env->ctx->descriptor_pool,
      .DescriptorPoolSize = 0,
      .MinImageCount = env->ctx->min_image_count,
      .ImageCount = env->ctx->fif,
      .PipelineCache = NULL,
      .PipelineInfoMain = {.RenderPass = env->ctx->renderpass,
                           .Subpass = 0,
                           .MSAASamples = VK_SAMPLE_COUNT_1_BIT},
      .UseDynamicRendering = false});
  igImplGlfw_InitForVulkan(env->wnd->handle, true);
  ImGuiIO* io = igGetIO_Nil();
  // io->MouseDrawCursor = true;

  for (int i = 0; i < NUM_FONT_SIZES; i++) {
    ImFontConfig icons_config = {.FontDataOwnedByAtlas = true,
                                 .OversampleH = 0,
                                 .OversampleV = 0,
                                 .GlyphMaxAdvanceX = FLT_MAX,
                                 .RasterizerDensity = 1,
                                 .RasterizerMultiply = 1,
                                 .EllipsisChar = 0,
                                 .MergeMode = true,
                                 .GlyphOffset = (ImVec2){0, 2 + i},
                                 .GlyphMinAdvanceX = 26.0f + i * 6};

    // Load fonts from resources (embedded or disk)
    // ImGui takes ownership of the data when FontDataOwnedByAtlas=true
    resource_t* mono_res = resource_load("fonts/mono_regular.ttf");
    if (mono_res) {
      usr->imgui_data.mono_font[i] = ImFontAtlas_AddFontFromMemoryTTF(
          io->Fonts, mono_res->data, mono_res->size, 20 + i * 4, NULL, NULL);
      free(mono_res); // Only free the resource struct, not the data
    }

    resource_t* icon_res = resource_load("fonts/iconfont.ttf");
    if (icon_res) {
      ImFontAtlas_AddFontFromMemoryTTF(
          io->Fonts, icon_res->data, icon_res->size, 20 + i * 4, &icons_config,
          (const ImWchar[]){0xe900, 0xeaea, 0});
      free(icon_res);
    }

    resource_t* regular_res = resource_load("fonts/regular_regular.ttf");
    if (regular_res) {
      usr->imgui_data.regular_font[i] = ImFontAtlas_AddFontFromMemoryTTF(
          io->Fonts, regular_res->data, regular_res->size, 20 + i * 4, NULL, NULL);
      free(regular_res);
    }

    icon_res = resource_load("fonts/iconfont.ttf");
    if (icon_res) {
      ImFontAtlas_AddFontFromMemoryTTF(
          io->Fonts, icon_res->data, icon_res->size, 20 + i * 4, &icons_config,
          (const ImWchar[]){0xe900, 0xeaea, 0});
      free(icon_res);
    }

    resource_t* mono_bold_res = resource_load("fonts/mono_bold.ttf");
    if (mono_bold_res) {
      usr->imgui_data.mono_font_bold[i] = ImFontAtlas_AddFontFromMemoryTTF(
          io->Fonts, mono_bold_res->data, mono_bold_res->size, 20 + i * 4, NULL, NULL);
      free(mono_bold_res);
    }

    icon_res = resource_load("fonts/iconfont.ttf");
    if (icon_res) {
      ImFontAtlas_AddFontFromMemoryTTF(io->Fonts, icon_res->data,
                                       icon_res->size, 20 + i * 4, &icons_config,
                                       (const ImWchar[]){0xe900, 0xeaea, 0});
      free(icon_res);
    }

    resource_t* regular_bold_res = resource_load("fonts/regular_bold.ttf");
    if (regular_bold_res) {
      usr->imgui_data.regular_font_bold[i] = ImFontAtlas_AddFontFromMemoryTTF(
          io->Fonts, regular_bold_res->data, regular_bold_res->size, 20 + i * 4, NULL, NULL);
      free(regular_bold_res);
    }

    icon_res = resource_load("fonts/iconfont.ttf");
    if (icon_res) {
      ImFontAtlas_AddFontFromMemoryTTF(io->Fonts, icon_res->data,
                                       icon_res->size, 20 + i * 4, &icons_config,
                                       (const ImWchar[]){0xe900, 0xeaea, 0});
      free(icon_res);
    }
  }
  
  // Build the font atlas after adding all fonts
  igImFontAtlasBuildMain(io->Fonts);
  
  io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io->IniFilename = NULL;

  ImGuiStyle* style = igGetStyle();
  style->DockingNodeHasCloseButton = false;
  style->WindowMenuButtonPosition = ImGuiDir_None;
  style->TabCloseButtonMinWidthUnselected = -1;
  style->WindowBorderSize = style->FrameBorderSize = style->ChildBorderSize =
      style->PopupBorderSize = style->TabBorderSize = 1;
  style->FramePadding = (ImVec2){8, 8};
  style->ItemSpacing = (ImVec2){4, 4};
  style->ItemInnerSpacing = (ImVec2){4, 4};
  style->WindowPadding = (ImVec2){4, 4};
  style->GrabMinSize = 18;
  style->FrameRounding = style->TabRounding = style->ChildRounding =
      style->GrabRounding = style->PopupRounding = style->ScrollbarRounding =
          style->WindowRounding = style->TreeLinesRounding = 3;
  style->ScrollbarSize = 10;
  style->DockingSeparatorSize = 1;
  style->ScrollbarPadding = 1;
  style->CellPadding.x = 2;

  igStyleColorsDark(style);
  style->Colors[ImGuiCol_Text] = (ImVec4){0.89f, 0.89f, 0.89f, 1.00f};
  style->Colors[ImGuiCol_WindowBg] = style->Colors[ImGuiCol_PopupBg] =
      (ImVec4){0.20f, 0.20f, 0.20f, 1.00f};
  style->Colors[ImGuiCol_Border] = (ImVec4){0.00f, 0.00f, 0.00f, 1.00f};
  style->Colors[ImGuiCol_BorderShadow] = (ImVec4){1.00f, 1.00f, 1.00f, 0.18f};
  style->Colors[ImGuiCol_FrameBg] = (ImVec4){0.16f, 0.16f, 0.16f, 1.00f};
  style->Colors[ImGuiCol_TitleBgActive] = (ImVec4){0.12f, 0.12f, 0.12f, 1.00f};
  style->Colors[ImGuiCol_Button] = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
  style->Colors[ImGuiCol_ButtonHovered] = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
  style->Colors[ImGuiCol_Header] = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
  style->Colors[ImGuiCol_HeaderHovered] = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
  style->Colors[ImGuiCol_TabHovered] = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
  style->Colors[ImGuiCol_Tab] = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
  style->Colors[ImGuiCol_TabSelected] = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
  style->Colors[ImGuiCol_TabSelectedOverline] =
      (ImVec4){0.26f, 0.59f, 0.98f, 0.00f};
  style->Colors[ImGuiCol_TabDimmed] = (ImVec4){0.17f, 0.17f, 0.17f, 1.00f};
  style->Colors[ImGuiCol_TabDimmedSelected] =
      (ImVec4){0.23f, 0.23f, 0.23f, 1.00f};
  style->Colors[ImGuiCol_ModalWindowDimBg] = (ImVec4){0, 0, 0, 0.7f};
  style->Colors[ImGuiCol_FrameBgHovered] = (ImVec4){0.23f, 0.23f, 0.23f, 1.00f};
  style->Colors[ImGuiCol_FrameBgActive] = (ImVec4){0.12f, 0.12f, 0.12f, 1.00f};
  style->Colors[ImGuiCol_SliderGrab] = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
  style->Colors[ImGuiCol_SliderGrabActive] =
      (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
  style->Colors[ImGuiCol_ButtonActive] = (ImVec4){0.14f, 0.14f, 0.14f, 1.00f};
  style->Colors[ImGuiCol_HeaderActive] = (ImVec4){0.14f, 0.14f, 0.14f, 1.00f};
}

void imgui_prerender() {
  igImplVulkan_NewFrame();
  igImplGlfw_NewFrame();
  igNewFrame();
}

void imgui_render(VkCommandBuffer cmd) {
  igImplVulkan_RenderDrawData(igGetDrawData(), cmd, NULL);
}

void imgui_destroy() {
  igImplGlfw_Shutdown();
  igImplVulkan_Shutdown();
  igDestroyContext(NULL);
  resource_manager_cleanup();
}