from pathlib import Path

p = Path('Source/PluginEditor.cpp')
s = p.read_text()

old_paint = '''    drawPanel(g,{14,74,330,556},"STOCHASTIC FIELD / PERFORMANCE");drawPanel(g,{354,74,774,556},"LIVING PLANET / LIVE MATHEMATICAL STATE");drawPanel(g,{1138,74,248,268},"EVOLUTION / STRUCTURE");drawPanel(g,{1138,352,248,278},"VOICE / PROBABILITY STATE");drawPanel(g,{14,640,500,220},audioProcessor.isDrumMode()?"PERCUSSION ENVELOPE / INTERNAL":"AMPLITUDE ENVELOPE");drawPanel(g,{524,640,610,220},"PRESETS / FIELD MEMORY");drawPanel(g,{1144,640,242,220},"OUTPUT");drawStochasticGlobe(g,{370,92,742,520});drawEvolutionGraph(g,{1156,116,212,142});'''
new_paint = '''    drawPanel(g,{14,74,430,786},"STOCHASTIC FIELD / PERFORMANCE");drawPanel(g,{454,74,650,556},"LIVING PLANET / LIVE MATHEMATICAL STATE");drawPanel(g,{1114,74,272,556},"PRESETS / FIELD MEMORY");drawPanel(g,{454,640,650,220},audioProcessor.isDrumMode()?"PERCUSSION ENVELOPE / INTERNAL":"AMPLITUDE ENVELOPE");drawPanel(g,{1114,640,272,220},"OUTPUT");drawStochasticGlobe(g,{470,92,618,520});'''

old_bounds = '''    ampWalk.setBounds(28,108,64,242);timeWalk.setBounds(104,108,64,242);ampMirror.setBounds(180,108,64,242);timeMirror.setBounds(256,108,64,242);\n    ampDist.setBounds(24,374,70,104);timeDist.setBounds(100,374,70,104);ampStep.setBounds(176,374,70,104);timeStep.setBounds(252,374,70,104);chaos.setBounds(24,490,70,104);breakpoints.setBounds(100,490,70,104);pitchStability.setBounds(176,490,70,104);curve.setBounds(252,490,70,104);boundary.setBounds(24,594,70,86);rate.setBounds(100,594,70,86);jump.setBounds(176,594,70,86);correlation.setBounds(252,594,70,86);orderButton.setBounds(25,686,68,20);\n    attack.setBounds(30,724,104,112);decay.setBounds(150,724,104,112);sustain.setBounds(270,724,104,112);release.setBounds(390,724,104,112);\n    seed.setBounds(548,700,86,92);fieldStatus.setBounds(646,716,148,35);presetNameEditor.setBounds(808,674,300,29);savePresetButton.setBounds(808,712,78,29);renamePresetButton.setBounds(894,712,82,29);deletePresetButton.setBounds(984,712,82,29);presetStatus.setBounds(808,754,300,47);voiceStatus.setBounds(1156,366,205,22);level.setBounds(1188,674,154,150);footerStatus.setBounds(415,872,570,14);'''
new_bounds = '''    ampWalk.setBounds(32,108,76,242);timeWalk.setBounds(132,108,76,242);ampMirror.setBounds(232,108,76,242);timeMirror.setBounds(332,108,76,242);\n    ampDist.setBounds(28,374,82,104);timeDist.setBounds(128,374,82,104);ampStep.setBounds(228,374,82,104);timeStep.setBounds(328,374,82,104);chaos.setBounds(28,490,82,104);breakpoints.setBounds(128,490,82,104);pitchStability.setBounds(228,490,82,104);curve.setBounds(328,490,82,104);boundary.setBounds(28,596,82,92);rate.setBounds(128,596,82,92);jump.setBounds(228,596,82,92);correlation.setBounds(328,596,82,92);orderButton.setBounds(30,706,82,22);\n    attack.setBounds(500,694,96,116);decay.setBounds(650,694,96,116);sustain.setBounds(800,694,96,116);release.setBounds(950,694,96,116);\n    seed.setBounds(1150,160,96,110);fieldStatus.setBounds(1248,184,120,35);presetNameEditor.setBounds(1146,310,210,29);savePresetButton.setBounds(1146,350,64,29);renamePresetButton.setBounds(1216,350,66,29);deletePresetButton.setBounds(1288,350,68,29);presetStatus.setBounds(1146,392,210,50);voiceStatus.setBounds(1146,456,210,22);level.setBounds(1172,686,156,150);footerStatus.setBounds(415,872,570,14);'''

if old_paint not in s:
    raise SystemExit('paint layout block not found')
if old_bounds not in s:
    raise SystemExit('bounds layout block not found')

s = s.replace(old_paint, new_paint)
s = s.replace(old_bounds, new_bounds)
p.write_text(s)
print('Applied reference-inspired compact layout')
