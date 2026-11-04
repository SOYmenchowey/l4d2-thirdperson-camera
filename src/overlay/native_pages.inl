static ImU32 Rgba(const ImVec4& color) { return ImGui::GetColorU32(color); }

static void DrawValueBadge(const char* text, const ImVec2& at, const ImVec2& size,
                           const ImVec4& bg, const ImVec4& fg) {
    const ImVec2 end(at.x + size.x, at.y + size.y);
    ImGui::GetWindowDrawList()->AddRectFilled(at, end, Rgba(bg), 4.0f);
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(at.x + (size.x - textSize.x) * 0.5f, at.y + (size.y - textSize.y) * 0.5f), Rgba(fg), text);
}

static void ValueBadgeItem(const char* text, const ImVec2& size, const ImVec4& bg, const ImVec4& fg) {
    DrawValueBadge(text, ImGui::GetCursorScreenPos(), size, bg, fg);
    ImGui::Dummy(size);
}

static void SectionHeader(const char* text) {
    ImGui::TextDisabled("%s", text);
    ImGui::Dummy(ImVec2(0, 6));
}

static bool TargetToggle(const char* id, const char* label, bool* value, const char* notice = nullptr) {
    ImGui::PushID(id);
    const float box = 28.0f, gap = 10.0f;
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const bool clicked = ImGui::InvisibleButton("##targetToggle", ImVec2(box + gap + textSize.x, box));
    if (clicked) {
        *value = !*value;
        NotificarCambio(notice ? notice : label, *value);
        Dirty();
    }
    const ImVec2 p = ImGui::GetItemRectMin();
    const ImVec2 end(p.x + box, p.y + box);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec4 on(0.55f, 0.86f, 0.72f, 1.0f);
    const ImVec4 offFill(0.105f, 0.113f, 0.125f, 1.0f);
    const ImVec4 offBorder(0.19f, 0.20f, 0.22f, 1.0f);
    dl->AddRectFilled(p, end, Rgba(*value ? on : offFill), 4.0f);
    dl->AddRect(p, end, Rgba(*value ? on : offBorder), 4.0f, 0, 1.0f);
    if (*value) {
        const ImVec2 a(p.x + 7.0f, p.y + 14.5f), b(p.x + 12.0f, p.y + 20.0f), c(p.x + 21.5f, p.y + 9.0f);
        const ImU32 white = Rgba(ImVec4(1, 1, 1, 1));
        const ImU32 outline = Rgba(ImVec4(0.025f, 0.028f, 0.032f, 1));
        dl->AddLine(a, b, outline, 4.4f); dl->AddLine(b, c, outline, 4.4f);
        dl->AddLine(a, b, white, 2.4f); dl->AddLine(b, c, white, 2.4f);
    }
    dl->AddText(ImVec2(p.x + box + gap, p.y + (box - ImGui::GetFontSize()) * 0.5f), Rgba(C_TXT), label);
    ImGui::PopID();
    return clicked;
}

static void BindButton(const char* id, int& key, bool& capturing) {
    ImGui::PushID(id);
    const bool wasCapturing = capturing;
    if (wasCapturing) ImGui::PushStyleColor(ImGuiCol_Text, C_ACCENT);
    const std::string label = wasCapturing ? "Presiona una tecla..." : NombreTecla(key);
    if (ImGui::Button(label.c_str(), ImVec2(104.0f, 30.0f))) {
        const bool start = !wasCapturing;
        CancelarCaptura();
        capturing = start;
    }
    if (wasCapturing) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hotkey de tercera persona. Esc cancela.");
    ImGui::PopID();
}

static bool TrackSlider(float* value, float minimum, float maximum, const char* id) {
    ImGui::PushID(id);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 4));
    const ImGuiCol hidden[] = { ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive,
        ImGuiCol_SliderGrab, ImGuiCol_SliderGrabActive };
    for (const ImGuiCol color : hidden) ImGui::PushStyleColor(color, ImVec4(0, 0, 0, 0));
    ImGui::SetNextItemWidth(-1);
    const bool changed = ImGui::SliderFloat("##distance", value, minimum, maximum, "", ImGuiSliderFlags_NoInput);
    ImGui::PopStyleColor(5); ImGui::PopStyleVar();
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    const float left = a.x + 2.0f, right = b.x - 2.0f, y = (a.y + b.y) * 0.5f;
    const float ratio = (*value - minimum) / (maximum - minimum);
    const float x = left + ImClamp(ratio, 0.0f, 1.0f) * (right - left);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine(ImVec2(left, y), ImVec2(right, y), Rgba(C_BORDER), 3.0f);
    dl->AddCircleFilled(ImVec2(x, y), 7.0f, Rgba(C_ACCENT), 16);
    ImGui::PopID();
    return changed;
}

static void NativeCamera() {
    SectionHeader("CAMARA / TERCERA PERSONA");
    const float maxX = ImGui::GetContentRegionMax().x;
    if (TargetToggle("thirdPerson", "Activada", &tp::g_activado, "Tercera persona")) Dirty();
    ImGui::SameLine(0, 0);
    ImGui::SetCursorPosX(maxX - 104.0f);
    BindButton("tpKey", g_tpHotkey, g_tpCapturando);

    ImGui::Dummy(ImVec2(0, 14));
    ImGui::TextUnformatted("Distancia");
    ImGui::SameLine(0, 0);
    char distanceText[32]; sprintf_s(distanceText, "%.0f", tp::g_distancia);
    ImGui::SetCursorPosX(maxX - 58.0f);
    ValueBadgeItem(distanceText, ImVec2(58.0f, 28.0f), C_PANEL, C_TXT);
    ImGui::Dummy(ImVec2(0, 2));
    if (TrackSlider(&tp::g_distancia, tp::DISTANCIA_MIN, tp::DISTANCIA_MAX, "tpDistance")) Dirty();
    tp::g_distancia = tp::LimitarDistancia(tp::g_distancia);

    if (tp::HayError()) {
        ImGui::Spacing();
        ImGui::TextColored(C_ERROR, "No se pudo aplicar la camara.");
    }
}

static void DrawNative(float alpha, float slide) {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 size(ImMin(355.0f, ImMax(280.0f, display.x - 16.0f)),
                      ImMin(300.0f, ImMax(260.0f, display.y - 16.0f)));
    if (g_panelPos.x < 0) g_panelPos = ImVec2((display.x - size.x) * 0.5f, (display.y - size.y) * 0.5f);
    g_panelPos.x = ImClamp(g_panelPos.x, 0.0f, ImMax(0.0f, display.x - size.x));
    g_panelPos.y = ImClamp(g_panelPos.y, 0.0f, ImMax(0.0f, display.y - size.y));

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::SetNextWindowPos(ImVec2(g_panelPos.x, g_panelPos.y + slide), ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::Begin("Thirdperson", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##drag", ImVec2(size.x - 46.0f, 46.0f));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        g_panelPos.x = ImClamp(g_panelPos.x + delta.x, 0.0f, ImMax(0.0f, display.x - size.x));
        g_panelPos.y = ImClamp(g_panelPos.y + delta.y, 0.0f, ImMax(0.0f, display.y - size.y));
    }
    ImGui::SetCursorPos(ImVec2(size.x - 40.0f, 7.0f));
    if (ImGui::Button("X##close", ImVec2(30.0f, 30.0f))) Cerrar();
    ImGui::SetCursorPos(ImVec2(14.0f, 5.0f));
    ImGui::TextUnformatted("L4D2 Third Person");
    ImGui::SetCursorPos(ImVec2(14.0f, 25.0f));
    ImGui::TextDisabled("@tokyossz");

    const float footerH = 62.0f;
    const float bodyY = 58.0f;
    const float bodyH = ImMax(120.0f, size.y - bodyY - footerH);
    ImGui::SetCursorPos(ImVec2(14.0f, bodyY));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 7.0f));
    ImGui::BeginChild("##cameraOptions", ImVec2(size.x - 28.0f, bodyH), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    NativeCamera();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    const float bottom = size.y - footerH;
    ImGui::SetCursorPos(ImVec2(14.0f, bottom));
    ImGui::Separator();
    ImGui::SetCursorPos(ImVec2(14.0f, bottom + 7.0f));
    if (ImGui::Button("Guardar", ImVec2(82.0f, 28.0f))) GuardarConfig();
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(g_saveError ? C_ERROR : g_dirty ? C_ACCENT : C_DIM, "%s",
        g_saveError ? "Error al guardar" : g_dirty ? "Sin guardar" : g_saved ? "Guardado" : "Preset actual");
    ImGui::SetCursorPos(ImVec2(14.0f, bottom + 36.0f));
    ImGui::TextDisabled("Insert: menu");
    ImGui::End();
    ImGui::PopStyleVar();
}
