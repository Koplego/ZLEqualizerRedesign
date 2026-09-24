# Node-driven liquid glass

The unlit material is neutral smoked glass. Active EQ nodes are the only chromatic light sources. The shell reads each displayed node's actual component coordinates, so its illumination follows the interactive position through dragging, filter changes, and editor resizing. Bypassed bands emit no light. Meter colors retain their signal-level meaning.

This pass replaces painted colored edge strokes and caustics with a scene-sampling glass compositor. The main panel renders its live node-lit backdrop into an image; the graph's actual grid surface is composited over it. Thin strips at the shell, graph, footer, meter, preset, speed, and phase boundaries sample displaced pixels from that scene. Normal displacement bends light inward at the edge, tangential displacement varies around the pane, and a neutral Fresnel highlight/absorption pair gives the material thickness. Because the sampled pixels come from the live node field, edge color and refracted grid detail change with the node positions without any fixed rainbow gradient or band-index color field.

The glass bodies have clear centers, a small neutral contact shadow, an asymmetric bright upper rim, and a restrained dark lower contour. The graph and footer remain transparent enough for the live illumination to show through. The central graph, response curves, node controls, analyzer, and audio processing retain their existing layout and behavior.

EQ node lenses now use the same material language. Their cores are translucent instead of solid colored discs; their annular edges sample radially displaced pixels from the live scene, and a curved white crown gives the small lenses a raised surface without an isolated specular dot. Pre, Post, and Side buttons use neutral glass pills with clearer selected and hover depth. Analyzer enablement has a separate, localized blue indicator in its label and switch; blue appears only while the analyzer is active, independent of the node-driven lighting across the rest of the interface.

## Validation — macOS arm64, REAPER 7.80, 24 September 2026

- Built the Release VST3 and installed it in the user's local VST3 folder, then reopened the actual plugin in REAPER.
- Moved the blue bell from 1.39 kHz / +5.75 dB to 354 Hz / -2.98 dB. The blue field followed its node, leaving the upper middle and illuminating the lower, leftward graph area. Restoring the node restored its earlier field.
- Bypassed the blue band. Its node, curve, and surrounding blue transmission became neutral while the other three active bands retained their colors. Re-enabling restored blue illumination.
- Played the test project through the plugin. The analyzer and stereo output meter both rendered live audio over the glass surface.
- Inspected successive installed builds against the supplied EQ reference and Apple Liquid Glass examples. The final pass reduces dark control fills and keeps refraction concentrated at the surface boundaries.
- Rebuilt and reopened the node-lens revision in REAPER. Moving the blue lens from 2.49 kHz / +6.72 dB to 1.17 kHz / +2.44 dB moved its rim refraction and broad light field; restoring the lens restored its position. Pre toggled independently. Turning the analyzer off removed the blue label/switch glow and disabled Pre/Post; turning it back on restored both the blue indication and the previous Pre/Post state.

This is a lightweight two-dimensional optical approximation in JUCE, not a ray-traced material or Apple's own Liquid Glass implementation. The live analyzer varies with input audio and playback time. Intel macOS and Windows builds were not tested in this pass.
