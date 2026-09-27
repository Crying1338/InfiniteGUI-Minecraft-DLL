#pragma once

class ClickEffectBase{
public:
	virtual bool Draw(ImDrawList* draw_list, const float& dt) = 0;
	virtual bool IsFinished() const = 0;
protected:

};
