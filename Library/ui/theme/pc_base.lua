--!NEEDS:../uitheme.lua

local colNone=UI.Colors.transparent
UI.Theme.PointCore_Base={
	-- Statics
	--colTesting
	["theme.SHADOW"]=UI.Color(0,0,0,0.7), --colShadow
	colShadow="theme.SHADOW",
	["theme.NIGHT"]=UI.Color(0,0,0,0.5), -- Unused yet in base widgets, for completeness
	["theme.NONE"]=UI.Color(0,0,0,0), -- Unused yet in base widgets, for completeness
	["theme.WHITE"]=UI.Color(1,1,1,1),
	["theme.WHITE70"]=UI.Color(1,1,1,.7),
	["theme.WHITE50"]=UI.Color(1,1,1,.5),
	["theme.WHITE30"]=UI.Color(1,1,1,.3),
	["theme.WHITE15"]=UI.Color(1,1,1,.15),
	["theme.BLACK"]=UI.Color(0,0,0,1),
	["theme.BLACK70"]=UI.Color(0,0,0,.7),
	["theme.BLACK50"]=UI.Color(0,0,0,.5),
	["theme.BLACK30"]=UI.Color(0,0,0,.3),
	["theme.BLACK15"]=UI.Color(0,0,0,.15),
	--Inactive backgrounds
	["theme.C1F"]=UI.Color(0x1B1B1B), --colDeep
	["theme.C1M"]=UI.Color(0x424242), --colBackground
	["theme.C1L"]=UI.Color(0x6D6D6D), --colPane
	colDeep="theme.C1F",
	colBackground="theme.C1M",
	colPane="theme.C1L",
	-- Active backgrounds
	["theme.C2F"]=UI.Color(0x005662), --colTile
	["theme.C2F30"]=UI.Color(0x005662,.3), --TODO --["theme.C2F30"]="=theme.C2F:alpha:.3", --GIDEROS 2024.3
	["theme.C2M"]=UI.Color(0x00838F), --colHeader
	["theme.C2L"]=UI.Color(0x4FB3BF), --colSelect
	colTile="theme.C2F",
	colHeader="theme.C2M",
	colSelect="theme.C2L",
	-- Composites
	["theme.C1MM"]=UI.Color(0x2E2E2E),
	["theme.C2FF"]=UI.Color(0x2E474B),
	-- Highlight
	["theme.CH"]=UI.Color(0xFFA726), --colHighlight
	colHighlight="theme.CH",
	-- Texts
	["theme.T"]=UI.Colors.white, --colText --colUI --T = T1 ( = T2 ? )
	["theme.TI"]=UI.Colors.black,
	colText="theme.T",
	colUI="theme.T",
	-- Base
	["theme.OV15"]=UI.Color(1,1,1,0.15),
	["theme.OV30"]=UI.Color(1,1,1,0.3), --colDisabled
	colDisabled="theme.OV30",
	["theme.OV50"]=UI.Color(1,1,1,0.5),
	["theme.OV70"]=UI.Color(1,1,1,0.7), -- Unused yet in base widgets
	["theme.OVI15"]=UI.Color(0,0,0,0.15),
	["theme.OVI30"]=UI.Color(0,0,0,0.3),
	["theme.OVI50"]=UI.Color(0,0,0,0.5),
	["theme.OVI70"]=UI.Color(0,0,0,0.7), -- Unused yet in base widgets							   
	-- States
	["theme.KO"]=UI.Color(0xFF0000), --colError
	colError=UI.Color(0xFF0000),
	--
	["accordion.colSelected"]=colNone,
	--
	["button.styBack"]={
		colWidgetBack="button.colBackground",
		brdWidget={ },
		shader=
			{ 
			class="UI.Shader.Shadow", 
			params={ radius=5, ccolor={0,0,0,1}, angle=180, displace=0, expand=1.05 } 
		},
	},
	["button.styInside"]={
		colWidgetBack=colNone,
		brdWidget={ },
		shader={}
	},
	["button.colBackground"]="theme.C2L",
	--
	["checkbox.colTickboxBack"]="theme.C2F",
	["checkbox.styDisabled"]={
		["checkbox.colTickboxBack"]="theme.OVI30",
		["checkbox.colTickboxFore"]="theme.OV30",
		["checkbox.colTickboxTick"]=colNone, 
	},
	["checkbox.styDisabledTicked"]={
		["checkbox.colTickboxBack"]="theme.OVI30",
		["checkbox.colTickboxFore"]="theme.OV30", 
		["checkbox.colTickboxTick"]="theme.OV30", 
	},
	["checkbox.styDisabledThird"]={
		["checkbox.colTickboxBack"]="theme.OVI30",
		["checkbox.colTickboxFore"]="theme.OV30", 
		["checkbox.colTickboxTick"]=colNone, 
		["checkbox.colTickboxThird"]="theme.OV30", 
	},
	--
	["chart.colItem"]="theme.C2M",
	["chart.colAxis"]="theme.C1L",
	["combobox.colBackground"]="theme.C2F",
	["combobox.colBorder"]="theme.C2M",
	--
	["breadcrumbs.styRoot"]={ 
		colText="theme.C2L",
	},
	["breadcrumbs.styItem"]={
		colText="theme.C2L",
	},
	["breadcrumbs.styLast"]={
		colText="colHighlight",
	},
	["breadcrumbs.styElipsis"]={
		colText="colUI",
	},
	["breadcrumbs.stySeparator"]={
		colText="colUI",
	},
	--
	["buttontextfield.szInset"]="0.25s",
	["buttontextfield.szMargin"]="0.5s",
	["dropdown.styButton"]={
		["button.styBack"]={
			colWidgetBack=UI.Colors.white,
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/textfield-multi.png",true),
				corners={0,"buttontextfield.szMargin","buttontextfield.szMargin","buttontextfield.szMargin",63,63,63,63,},
				insets="buttontextfield.szMargin",
			}),
			shader={ 
				class="UI.Shader.MultiLayer", 
				params={ colLayer1="button.colBackground", colLayer2="button.colBorder", colLayer3="button.colFocus", colLayer4=colNone } 
			}
		},
		["button.colBackground"]="textfield.colBorder",
		["button.colBorder"]=colNone,
		["button.colFocus"]=colNone,
		["button.styInside"]={
			colWidgetBack=colNone,
			brdWidget={},
			shader={},
			["image.colTint"]="colText",
		},
		["button.styDisabled"]={
			["button.styInside"]={
				colWidgetBack=colNone,
				brdWidget={},
				shader={},
				["image.colTint"]="theme.OV30",
			},
		},
		["button.styError"]={
			["button.colBackground"]="theme.KO",
		},
		["button.stySelected"]={
			["button.colBackground"]="theme.C2L",
		},
		["button.stySelectedFocused"]={
			["button.colBackground"]="theme.C2L",
		},
		["button.styFocused"]={
			["button.colBackground"]="textfield.colBorder",
		},
	},
--[[	["buttontextfieldcombo.styButtonDisabled"]={
		["button.styBack"]={
			colWidgetBack=UI.Colors.white,
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/textfield-multi.png",true),
				corners={0,"textfield.szMargin","textfield.szMargin","textfield.szMargin",63,63,63,63,},
				insets="textfield.szMargin",
			}),
			shader={
				class="UI.Shader.MultiLayer", 
				params={ colLayer1="button.colBackground", colLayer2="button.colBorder", colLayer3="button.colFocus", colLayer4=colNone } 
			}
		},
		["button.colBackground"]=colNone,
		["button.colBorder"]=colNone,
		["button.colFocus"]=colNone,
		["button.styError"]={},
		["button.stySelected"]={},
		["button.stySelectedFocused"]={},
		["button.styFocused"]={},
	},]]
	--
	["calendar.colBackground"]="theme.C1M",
	["calendar.colBorder"]="theme.C2M",
	["calendar.colDaysOther"]="theme.OV50",
	["calendar.stySpinners"]={
		["spinner.colIcon"]="theme.OV50",
	},

	--	
	["dnd.colSrcHighlight"]="theme.OV15",
	["dnd.colDstHighlight"]="theme.T",
	["dnd.colDstHighlightOver"]="theme.OV50",
	--
	["hilitpanel.colHighlight"]="theme.OV15",
	--
	["passwordfield.styButtonDisabled"]={
		["button.styInside"]={
			["image.colTint"]="theme.OV30",
		},
	},
	--
	["keyboard.colSpecKeys"]="theme.C2F",
	["keyboard.styKeys"]={
		["button.styBack"]={
			colWidgetBack=UI.Colors.white,
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/textfield-multi.png",true),
				corners={"textfield.szMargin","textfield.szMargin","textfield.szMargin","textfield.szMargin",63,63,63,63,},
				insets="textfield.szMargin",
			}),
			shader={ 
				class="UI.Shader.MultiLayer", 
				params={ colLayer1="button.colBackground", colLayer2="theme.C2M", colLayer3=colNone, colLayer4=colNone } 
			}
		},
		["label.font"]="font.bold",
	},
	["keyboard.stySpecKeys"]={
		["button.styBack"]={
			colWidgetBack=UI.Colors.white,
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/textfield-multi.png",true),
				corners={"textfield.szMargin","textfield.szMargin","textfield.szMargin","textfield.szMargin",63,63,63,63,},
				insets="textfield.szMargin",
			}),
			shader={ 
				class="UI.Shader.MultiLayer", 
				params={ colLayer1="keyboard.colSpecKeys", colLayer2="theme.C2M", colLayer3=colNone, colLayer4=colNone } 
			}
		},	
	},
	--
	["progress.colBorder"]="theme.C1M",
	["progress.colBackground"]="theme.C1M",
	["progress.colRemain"]="theme.C2F",
	["progress.styDisabled"]={
		["progress.colRemain"]="theme.C1F",
		["progress.colBackground"]=colNone,
		["progress.colDone"]="theme.C1L",
	},
	--
	["scrollbar.colBar"]="theme.C1L",
	--
	["slider.colKnob"]=colNone,
	["slider.colKnobCenter"]="theme.C2L",
	["slider.colRailBorder"]="theme.C1M",
	["slider.colRailBackground"]="theme.C1L",
	["slider.colRailActive"]="theme.CH",
	["slider.styDisabled"]={
		["slider.colRailActive"]="theme.OVI30",
		["slider.colRailBackground"]="theme.OVI30",
	},
	["slider.styKnobDisabled"]={
		["slider.colKnobCenter"]="theme.OV30",
		colText="theme.OV30",
	},
	--
	["spinner.styButtonDisabled"]={
		["image.colTint"]="theme.OV30",
	},
	--
	["splitpane.styThin"]={
		-- Image is 51 wide but centered in a canvas of 128, 
		-- so around 40% which should be fitted in tless than the center bar.
		-- Reduce knob size to 80% for better look
		["splitpane.szKnob"]=".8is", 
		["splitpane.tblKnobSizes"]={".8is",".4is",".8is"},
		["splitpane.colKnob"]=UI.Colors.transparent,
		["splitpane.colKnobBackground"]="theme.C1F",
		["splitpane.styKnobBackgroundH"]={
			colWidgetBack="splitpane.colKnobBackground",
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/knob-band-v.png",true,{ mipmap=true }),
				corners={0,0,"1s","1s",0,0,63,63},
			})
		},
		["splitpane.styKnobBackgroundV"]={
			colWidgetBack="splitpane.colKnobBackground",
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/knob-band-h.png",true,{ mipmap=true }),
				corners={"1s","1s",0,0,63,63,0,0},
			})
		},
		["splitpane.styKnobH"]={
			colWidgetBack="splitpane.colKnob" ,
			brdWidget={},
		},
		["splitpane.styKnobV"]={
			colWidgetBack="splitpane.colKnob" ,
			brdWidget={},
		},
		["splitpane.styKnobHandleH"]={
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/knob-dots-v.png",true,{ mipmap=true }),
				corners={0,0,0,0,0,0,0,0},
				insets="splitpane.szKnob",
			}),
			colWidgetBack="splitpane.colKnobHandle",
		},
		["splitpane.styKnobHandleV"]={
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/knob-dots-h.png",true,{ mipmap=true }),
				corners={0,0,0,0,0,0,0,0},
				insets="splitpane.szKnob",
			}),
			colWidgetBack="splitpane.colKnobHandle",
		},
	},
	--
	["tabbedpane.colBackground"]="theme.C1F",
	--["tabbedpane.colOutline"]="theme.CH",
	["tabbedpane.colTabBackground"]="theme.C2F",
	["tabbedpane.colTabBorder"]="theme.C2M",	
	--
	["table.colHeader"]="theme.C2F",
	["table.colTextHeader"]="theme.CH",
	["table.styDndMarker"]={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/textfield-multi.png",true),
			corners={"textfield.szMargin","textfield.szMargin","textfield.szMargin","textfield.szMargin",63,63,63,63,},
			insets="textfield.szMargin",
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="table.colHeader", colLayer2="dnd.colDstHighlight", colLayer3=colNone, colLayer4=colNone }
		},
		colWidgetBack="dnd.colDstHighlight",
		colText="table.colTextHeader",
	},
	["table.styRowOdd"]={},
	["table.styRowSelected"]={},
	["table.styRowEven"]="table.styRowOdd",
	["table.styCellSelected"]={
		colWidgetBack = "colSelect"
	},
	--
	["textfield.colTipText"]="theme.OV50",
	["textfield.colBackground"]="theme.C2F", 	--R
	["textfield.colBorder"]="theme.C2M", 		--V
	["textfield.colBorderWide"]=colNone,		--B
	["textfield.styDisabled"]={
		["textfield.colBackground"]=colNone, 
		["textfield.colBorder"]="theme.OV30", 
		["textfield.colBorderWide"]=colNone, 
		["textfield.colForeground"]="theme.OV50",
	},
	["textfield.styError"]={
		["textfield.colForeground"]="theme.KO",
	},
	["textfield.styErrorFocused"]={
		["textfield.colBorder"]=colNone,
		["textfield.colBorderWide"]="theme.C2L",
		["textfield.colForeground"]="theme.KO",
	},
	["textfield.styErrorFocusedReadonly"]="styErrorReadonly",
	["textfield.styErrorReadonly"]={
		["textfield.colBackground"]=colNone, 
		["textfield.colBorder"]="theme.OV30",
		["textfield.colForeground"]="theme.KO",
	},
	["textfield.styFocused"]={
		["textfield.colBorder"]=colNone, 
		["textfield.colBorderWide"]="theme.C2L", 
	},
	["textfield.styFocusedReadonly"]="textfield.styReadonly",
	["textfield.styReadonly"]={
		["textfield.colBackground"]=colNone, 
		["textfield.colBorder"]="theme.OV30", 
	},
	["textfield.szMargin"]=".5s",
	["textfield.colCutPasteButton"]="theme.C2M",
	--
	["toolbox.colHeaderIcon"]="theme.OV50",
	["toolbox.colHeader"]="theme.C1F",
	["toolbox.colBorder"]="theme.C1F",
	["toolbox.colBack"]="theme.C2F",
	--
	["tooltip.szIcon"]="2is",
	["tooltip.szMarker"]="2is",
	["tooltip.szUrlDisplace"]=0.2,
	["tooltip.szMinWidth"]="10em",
	["tooltip.icIcon"]=Texture.new("ui/icons/ic_help.png",true,{ mipmap=true, rawalpha=true }),
	["tooltip.icUrl"]=Texture.new("ui/icons/ic_ref.png",true,{ mipmap=true, rawalpha=true }),
	["tooltip.colIcon"]="colHighlight",
	["tooltip.colIconFore"]="theme.C1F",
	["tooltip.colUrl"]="theme.C2L",
	["tooltip.colUrlFore"]="theme.T",
	["tooltip.colRef"]="theme.C2L",
	["tooltip.colShadow"]="theme.SHADOW",
	["tooltip.colMarkerBack"]="theme.C1F",
	["tooltip.styLargeMarker"]={
		colWidgetBack=UI.Colors.white,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"1is","1is","1is","1is",63,63,63,63,},
			insets=0,
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="tooltip.colMarkerBack", colLayer2="tooltip.colMarkerBorder", colLayer3=colNone, colLayer4=colNone }
		},
	},

	["tooltip.styIcon"]={
		colWidgetBack=UI.Colors.white,
		brdWidget=UI.Border.NinePatch.new({
			texture="tooltip.icIcon",
			corners={0,0,0,0,0,0,0,0},
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="tooltip.colIcon", colLayer2="tooltip.colIconFore", colLayer3=colNone, colLayer4=colNone }
		},
	},
	["tooltip.styHelpMarker"]={
		colWidgetBack=UI.Colors.white,
		brdWidget=UI.Border.NinePatch.new({
			texture="tooltip.icIcon",
			corners={0,0,0,0,0,0,0,0},
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1=colNone, colLayer2="tooltip.colIcon", colLayer3=colNone, colLayer4=colNone }
		},
	},
	["tooltip.styUrl"]={
		colWidgetBack=UI.Colors.white,
		brdWidget=UI.Border.NinePatch.new({
			texture="tooltip.icUrl",
			corners={0,0,0,0,0,0,0,0},
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="tooltip.colUrl", colLayer2="tooltip.colUrlFore",  colLayer3="tooltip.colShadow", colLayer4=colNone }
		},
	},
	["tooltip.styRef"]={
		--font="font.small",
		["label.color"]="tooltip.colRef",
	},
}
UI.Theme.PointCore_Base.__index=UI.Theme.PointCore_Base

UI.Theme.PointCore_Red={
	["theme.C2F"]=UI.Color(0x8E0000),
	["theme.C2M"]=UI.Color(0xC62828),
	["theme.C2L"]=UI.Color(0xFF5F52),
	["theme.C2FF"]=UI.Color(0x4B472E),
	["theme.CH"]=UI.Color(0x26A7FF),
}
setmetatable(UI.Theme.PointCore_Red,UI.Theme.PointCore_Base)

UI.Theme.PointCore_Pink={
	["theme.T"]=UI.Colors.black,
	["theme.C1F"]=UI.Color(0xBCBCBC),
	["theme.C1M"]=UI.Color(0xEEEEEE),
	["theme.C1L"]=UI.Color(0xFFFFFF),
	["theme.C1MM"]=UI.Color(0xAEAEAE),
	["theme.C2F"]=UI.Color(0xBF5F82),
	["theme.C2M"]=UI.Color(0xF48FB1),
	["theme.C2L"]=UI.Color(0xFFC1E3),
	["theme.C2FF"]=UI.Color(0x4B472E),
	["theme.CH"]=UI.Color(0x43A047),
	["theme.OV15"]=UI.Color(0,0,0,0.15),
	["theme.OV30"]=UI.Color(0,0,0,0.3),
	["theme.OV50"]=UI.Color(0,0,0,0.5),
}
setmetatable(UI.Theme.PointCore_Pink,UI.Theme.PointCore_Base)
