# Node-driven liquid glass

The unlit material is smoked neutral glass. Active EQ lenses are the chromatic sources. The main shell reads each displayed lens's actual component coordinates, so the optical source follows the same geometry as the interactive node, including shelf gain scaling and editor resizing.

Transmission decays in both axes. Glass boundaries receive narrow rim highlights and elongated grazing reflections, with energy attenuated by source-to-edge distance. Source color controls the stylized scattering radius; frequency does not select a fixed background color. Header/footer controls and the meter housing transmit this shared field. The floating inspector's rim uses the selected node's position in the inspector's local coordinates.

The glass now has neutral layered bevels: a bright outer contour, a displaced inner highlight, and a dark inner contour. Live sources project thin colored light into those edges. Short curved caustics bend inward from the shell, graph, footer, meter, preset, and footer controls. Projection uses each source's two-dimensional distance from the surface, so a distant node does not color an entire small control. The bevels remain colorless with all bands off; every colored caustic moves with or disappears with its source.

Bypassed/off bands emit no colored shell illumination. Bypassed lenses and individual response curves are neutral; their broad fills and local colored halos are suppressed. Meter bar colors retain their signal-level meaning. Spectrum geometry and EQ/audio processing are unchanged.

## Manual validation — macOS arm64, REAPER 7.80, 24 September 2026

Five compiled VST3 visual passes were installed and reopened in REAPER against the supplied image. Tests used the actual plugin and real audio, not an HTML reconstruction.

- A blue bell moved from 1.39 kHz / +5.75 dB to 42.5 Hz / -9.80 dB. Blue light left the upper-middle graph/header and appeared on the lower-left graph, controls and footer. Restoring the node restored the field.
- Bypassing the blue band removed its local bloom, broad transmission and boundary reflections. Re-enabling restored them.
- Bypassing all four bands returned the shell, graph, footer, meter housing and lenses to neutral glass (second pass; the same active-source gate is retained in the final pass).
- Audio playback drove the analyzer and stereo output meter. The analyzer now has a thin neutral outline and translucent fill instead of the previous opaque white mass.
- Switching filter type retains a single clean slope label; the invisible parameter control no longer restores its own text alpha. Inspector rim lighting repaints during movement.
- Final VST3 build succeeded. The installed executable's SHA-256 matched the built executable, and its ad-hoc signature verified.

The reference's body proportions and panel layout are retained, with its compact selected-band badge restored. This is a stylized distance-based optical model, not a ray-traced material. The live analyzer naturally differs with input audio and playback time. Intel macOS and Windows builds were not tested in this pass.

## Refraction follow-up — macOS arm64, REAPER 7.80, 24 September 2026

Three additional compiled VST3 passes were installed and inspected live. The first made graph and shell edge caustics visible. The second exposed over-bright multicolor rims on small controls; the third narrowed those rims and attenuated source light by full two-dimensional distance. Moving the blue bell from 1.39 kHz / +5.75 dB to 205 Hz / -5.86 dB moved its graph and footer edge reflections. Bypassing the blue bell removed blue light from the preset, graph, shell, and footer while the neutral bevels remained; re-enabling restored them. The final pass was tested with real audio driving the analyzer and stereo meter.
