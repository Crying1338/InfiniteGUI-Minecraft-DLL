#pragma once


struct RenderTask
{
	bool gui = true;
	bool before = false;
	bool after = false;
	void clear()
	{
		gui = false;
		before = false;
		after = false;
	}
};

struct RenderDirtyState
{
	bool contentDirty = false;
	bool animating = false;
	void clear()
	{
		contentDirty = false;
		animating = false;
	}
};

class RenderModule
{
public:
	virtual void RenderGui() = 0;
	virtual void RenderBeforeGui() = 0;
	virtual void RenderAfterGui() = 0;

	bool IsRenderGui() const { return renderTask.gui; }
	bool IsRenderBeforeGui() const { return renderTask.before; }
	bool IsRenderAfterGui() const { return renderTask.after; }

	bool IsContentDirty() const { return dirtyState.contentDirty; }
	void SetContentDirty(bool dirty) { dirtyState.contentDirty = dirty; }
	bool IsAnimating() const { return dirtyState.animating; }
protected:
	RenderTask renderTask;
	RenderDirtyState dirtyState;
};