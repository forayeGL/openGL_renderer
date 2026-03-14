#pragma once
#include "../render_context.h"
#include "../../framebuffer/framebuffer.h"

/**
 * @brief 渲染Pass基类
 * 
 * 所有渲染管线中的Pass都继承此基类。
 * 每个Pass代表管线中的一个独立阶段（如阴影、GBuffer填充、光照计算、后处理等）。
 * Pass之间通过Framebuffer传递数据，实现低耦合的管线架构。
 * 
 * 设计原则：
 * - 每个Pass只负责单一职责
 * - Pass之间通过输入/输出FBO解耦
 * - 统一的生命周期管理（init → execute → destroy）
 */
class RenderPass {
public:
	virtual ~RenderPass() = default;

	/**
	 * @brief 初始化Pass所需资源（FBO、Shader等）
	 * @param width  帧缓冲宽度
	 * @param height 帧缓冲高度
	 * 在OpenGL上下文就绪后调用一次
	 */
	virtual void init(int width, int height) = 0;

	/**
	 * @brief 执行此Pass的渲染逻辑
	 * @param ctx 当前帧的渲染上下文（场景、相机、光源等）
	 * 每帧调用一次
	 */
	virtual void execute(const RenderContext& ctx) = 0;

	/**
	 * @brief 获取Pass名称（用于调试和日志）
	 * @return Pass的可读名称
	 */
	virtual const char* getName() const = 0;
};
