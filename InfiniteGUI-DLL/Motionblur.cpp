#include "Motionblur.h"
#include <GL/glew.h>
#include <GL/GL.h>

#include "FpsItem.h"
#include "opengl_hook.h"
#include "GameStateDetector.h"
#include "imgui\imgui_internal.h"
#include "ImGuiStd.h"
auto vertex_shader_code = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)glsl";

auto fragment_shader_code = R"glsl(
#version 330 core

out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D currentTexture;
uniform sampler2D historyTexture;
uniform float blurriness;
uniform float velocity_factor;
uniform bool renderRGB;
uniform bool smooth_blur;

vec4 blurHistory(vec2 uv)
{
    float offset = 0.0006 * velocity_factor;
    vec4 sum = vec4(0.0);

    sum += texture(historyTexture, uv + vec2(-offset, 0.0)) * 0.25;
    sum += texture(historyTexture, uv + vec2( offset, 0.0)) * 0.25;
    sum += texture(historyTexture, uv + vec2(0.0, -offset)) * 0.25;
    sum += texture(historyTexture, uv + vec2(0.0,  offset)) * 0.25;

    return sum;
}

void main()
{
    vec4 current = texture(currentTexture, TexCoord);
    vec4 history = texture(historyTexture, TexCoord);
    float cur_blurriness = blurriness;
    vec4 blurredHistory = history;
	if(velocity_factor > 0.0){
		float base_blurriness = blurriness * 0.5;
		cur_blurriness = base_blurriness + velocity_factor * base_blurriness;
		if(smooth_blur)
		{
			blurredHistory = blurHistory(TexCoord);
		}
	}
	vec4 blurredColor = mix(current, blurredHistory, cur_blurriness);
    
    if (renderRGB)
    {
        FragColor = blurredColor;
    }
    else
    {
		float value1 = current.r;
		FragColor = mix(vec4(value1), blurredHistory, cur_blurriness);
    }
}

)glsl";

void Motionblur::Toggle()
{
	if(!isEnabled)
		Destroy();
}

void Motionblur::RenderGui()
{

}

void Motionblur::RenderBeforeGui()
{
	Render();
}

void Motionblur::RenderAfterGui()
{
	Render();
}

void Motionblur::Render()
{
	if (!isEnabled) return;
	if (!applyOnGameMenu && GameStateDetector::Instance().GetCurrentState() == InGameMenu)
		return;
	static bool first = true;

	const int width = opengl_hook::screen_size.x;
	const int height = opengl_hook::screen_size.y;

	GLint viewport[4];
	GLint prev_framebuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_framebuffer);
	glGetIntegerv(GL_VIEWPORT, viewport);

	glViewport(0, 0, width, height);

	if (!initialized_)
	{
		initialize_texture(width, height);
		initialize_quad();
		initialize_shader();
		initialized_ = true;

		texture_width_ = width;
		texture_height_ = height;
	}

	if (texture_width_ != width || texture_height_ != height)
	{
		resize_texture(width, height);
		first = true;
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	copy_to_current();
	if (first)
	{
		copy_to_history();
		first = false;
	}

	//计算blurriness_value值
	if (velocityAdaptive)
		velocity_adaptive_blur(GameStateDetector::Instance().IsCameraMoving(), GameStateDetector::Instance().GetCameraSpeed(), &velocity_factor);
	else
		velocity_factor = 1.0f;

	if (FpsModulate)
		Fps_modulate(FpsItem::Instance().GetInstantaneousFPS(), &blurriness_value, &cur_blurriness_value);
	else
		cur_blurriness_value = blurriness_value;

	draw_texture();
	copy_to_history();

	glBindFramebuffer(GL_FRAMEBUFFER, prev_framebuffer);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void Motionblur::Destroy()
{
	if(!initialized_) return;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
	glBindVertexArray(0);

	if (shader_program_)
		glDeleteProgram(shader_program_);

	if (current_texture_)
		glDeleteTextures(1, &current_texture_);

	if (history_texture_)
		glDeleteTextures(1, &history_texture_);

	if (quad_vao_)
		glDeleteVertexArrays(1, &quad_vao_);

	if (quad_vbo_)
		glDeleteBuffers(1, &quad_vbo_);

	shader_program_ = 0;
	current_texture_ = 0;
	history_texture_ = 0;
	quad_vao_ = 0;
	quad_vbo_ = 0;

	initialized_ = false;
}

void Motionblur::initialize_texture(const int width, const int height)
{
	glGenTextures(1, &current_texture_);
	glBindTexture(GL_TEXTURE_2D, current_texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &history_texture_);
	glBindTexture(GL_TEXTURE_2D, history_texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Motionblur::initialize_quad()
{
	constexpr GLfloat quad_vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};

	glGenVertexArrays(1, &quad_vao_);
	glGenBuffers(1, &quad_vbo_);

	glBindVertexArray(quad_vao_);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), static_cast<GLvoid*>(0));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
		reinterpret_cast<GLvoid*>(2 * sizeof(GLfloat)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Motionblur::initialize_shader()
{
	if (!shader_program_)
		shader_program_ = glCreateProgram();

	const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_code, nullptr);
	glCompileShader(vertex_shader);

	const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_code, nullptr);
	glCompileShader(fragment_shader);

	glAttachShader(shader_program_, vertex_shader);
	glAttachShader(shader_program_, fragment_shader);
	glLinkProgram(shader_program_);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
}

void Motionblur::resize_texture(int width, int height)
{
	glBindTexture(GL_TEXTURE_2D, current_texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, history_texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, 0);

	texture_width_ = width;
	texture_height_ = height;
}

void Motionblur::draw_texture() const 
{
	if (current_texture_ == 0)
		return;
	GLboolean depth, blend;
	glGetBooleanv(GL_DEPTH_TEST, &depth);
	glGetBooleanv(GL_BLEND, &blend);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glUseProgram(shader_program_);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, current_texture_);
	glUniform1i(glGetUniformLocation(shader_program_, "currentTexture"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, history_texture_);
	glUniform1i(glGetUniformLocation(shader_program_, "historyTexture"), 1);

	auto value = cur_blurriness_value / 11.0f;
	glUniform1f(glGetUniformLocation(shader_program_, "blurriness"), (GLfloat)value);
	glUniform1f(glGetUniformLocation(shader_program_, "velocity_factor"), (GLfloat)velocity_factor);
	glUniform1f(glGetUniformLocation(shader_program_, "renderRGB"), !clear_color);
	glUniform1f(glGetUniformLocation(shader_program_, "smooth_blur"), smooth_blur);

	glBindVertexArray(quad_vao_);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);

	if (depth) glEnable(GL_DEPTH_TEST);
	if (blend) glEnable(GL_BLEND);
}

void Motionblur::copy_to_history() const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, history_texture_);
	glCopyTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0, 0,
		0, 0,
		texture_width_,
		texture_height_
	);
}

void Motionblur::copy_to_current() const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, current_texture_);
	glCopyTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0, 0,
		0, 0,
		texture_width_,
		texture_height_
	);
}

void Motionblur::velocity_adaptive_blur(bool cameraMoving, float cameraSpeed, float* velocity_factor)
{
	float tar_velocity_factor = std::clamp((cameraSpeed - 1.0f), 0.0f, 15.0f) / 15.0f;

	if (cameraMoving)
	{
		*velocity_factor = tar_velocity_factor;
	}
}

void Motionblur::Fps_modulate(const float& fps, const float* blurriness_value, float* cur_blurriness_value)
{
	float normalized_fps = std::clamp(fps, 0.0f, 1000.0f) / 1000.0f;  // 归一化到[0,1]

	// 指数衰减：fps越低，衰减越强
	// 调整power值可以控制曲线陡峭程度（越大则低FPS衰减越强）
	float attenuation_factor = std::pow(normalized_fps, 0.2f);

	*cur_blurriness_value = *blurriness_value * attenuation_factor;
}

void Motionblur::Load(const nlohmann::json& j)
{
	LoadItem(j);
	if (j.contains("blurriness")) blurriness_value = j["blurriness"].get<float>();
	//调整blurriness不超过10
	blurriness_value = std::clamp(blurriness_value, 0.0f, 10.0f);
	if (j.contains("velocityAdaptive")) velocityAdaptive = j["velocityAdaptive"].get<bool>();
	if (j.contains("smooth_blur")) smooth_blur = j["smooth_blur"].get<bool>();
	if (j.contains("applayOnMenu")) applyOnMenu = j["applayOnMenu"].get<bool>();
	if (j.contains("applyOnGameMenu")) applyOnGameMenu = j["applyOnGameMenu"].get<bool>();
	processApplyOnMenu();
	if (j.contains("clear_color")) clear_color = j["clear_color"].get<bool>();
	if (j.contains("FpsModulate")) FpsModulate = j["FpsModulate"].get<bool>();
}

void Motionblur::Save(nlohmann::json& j) const
{
	SaveItem(j);
	j["blurriness"] = blurriness_value;
	j["velocityAdaptive"] = velocityAdaptive;
	j["smooth_blur"] = smooth_blur;
	j["applyOnMenu"] = applyOnMenu;
	j["applyOnGameMenu"] = applyOnGameMenu;
	j["clear_color"] = clear_color;
	j["FpsModulate"] = FpsModulate;
}

void Motionblur::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{

	float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(bigItemWidth);
	//DrawItemSettings();
	ImGui::SliderFloat(u8"模糊强度", &blurriness_value, 0.0f, 10.0f, "%.1f");

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::Checkbox(u8"菜单中开启", &applyOnMenu))
	{
		processApplyOnMenu();
	}
	ImGui::SameLine();
	ImGui::SetCursorPosX(centerX + bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::Checkbox(u8"游戏菜单中开启", &applyOnGameMenu);
	ImGui::SameLine();ImGuiStd::HelpMarker(u8"打开背包、暂停、打字等非游戏时Gui的显示。");

	ImGui::SetCursorPosX(bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::Checkbox(u8"帧率调制", &FpsModulate);
	ImGui::SameLine(); ImGuiStd::HelpMarker(u8"模糊强度随着帧率变化，适用于帧率不稳定的情况。");
	ImGui::SameLine();
	ImGui::SetCursorPosX(centerX + bigPadding);
	ImGui::SetNextItemWidth(itemWidth);
	if (ImGui::Checkbox(u8"速度自适应", &velocityAdaptive))
	{
		if (!velocityAdaptive) smooth_blur = false;
	}
	ImGui::SameLine(); ImGuiStd::HelpMarker(u8"根据视角移动速度调整模糊强度，能有效解决鬼影问题。\n对于mc1.12及以下版本此功能将失效。");
	ImGui::SetCursorPosX(bigPadding);
	ImGui::Checkbox(u8"Im Faded~", &clear_color);

	if (velocityAdaptive) { 
		ImGui::SameLine();
		ImGui::SetCursorPosX(bigPadding + centerX);
		ImGui::SetNextItemWidth(itemWidth);

		ImGui::Checkbox(u8"柔和模糊", &smooth_blur); ImGui::SameLine(); ImGuiStd::HelpMarker(u8"柔和化模糊拖影，但会使MCUI模糊。"); 
	}
}
