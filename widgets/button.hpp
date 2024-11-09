#pragma once

#include "./common.hpp"
#include <string_view>

struct toggle_button 
{
	using info = constant
	<
		widget_info()
		.state_stored<bool>()
		.style<trigger_button::style>()
	>;
	
	using value_change = value_change<bool>;
	
	static vec2f size() { return {50, 20}; }
	
	toggle_button(std::string_view string)
	{
		text(string);
	}
	
	toggle_button(ignored, std::string_view string)
	: toggle_button{string}
	{}
	
	void text(std::string_view txt)
	{
		str = txt.data();
		sz = (unsigned) txt.size();
	}
	
	std::string_view text() const { return {str, (unsigned)sz}; }
	
	using context = widget_context<{(type)^state_write<bool>, (type)^state_read<bool>}>;
	
	void on(mouse_enter e, context ctx)
	{
		hovered = true;
		emit(ctx, repaint_request{});
	}
	
	void on(mouse_exit e, Ctx ctx)
	{
		hovered = false;
		emit(ctx, repaint_request{});
	}
	
	void on(mouse_up e, Ctx ctx)
	{
		active = not active;
		emit(ctx, repaint_request{});
		emit(ctx, state_write<bool>{active}, this);
	}
	
	void paint(painter& p, paint_data<toggle_button> d)
	{
		const auto& s = d.style();
		const auto box = d.bounds();
		
		p.fill_style( active ? s.active_col : s.inactive_col );
		p.fill( box, 3.f );
		
		if (hovered)
		{
			p.fill_style( highlight_col.with_alpha(0.2) );
			p.fill( box, 3.f );
		}
		
		p.stroke_style(colors::white.with_alpha(0.5));
		p.stroke(box, 1.f, 3.f);
		
		p.text_alignment( text_align::x::center, text_align::y::center );
		
		p.fill_style( s.text_col );
		p.fill( flux::text_at{text(), box.center()} );
	}
	
	private :
	
	const char* str;
	unsigned sz     : 30;
	bool active  : 1 = false;
	bool hovered : 1 = false;
};