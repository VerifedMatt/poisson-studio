# Third-party notices

Project source is licensed under GPL-2.0-only; see `LICENSE`. Third-party components remain under their respective terms.

| Component | Version/source | License and location |
| --- | --- | --- |
| Dear ImGui | 1.91.9b, github.com/ocornut/imgui | MIT; downloaded `vendor/imgui-1.91.9b/LICENSE.txt` |
| MySQL Community server/client | 8.4.13, dev.mysql.com | GPL v2 and included third-party notices; downloaded `tools/mysql-8.4.13-winx64/LICENSE` and documentation |
| w64devkit | 2.11.0, github.com/skeeto/w64devkit | Collection of separately licensed tools; upstream distribution/source and included notices apply |
| nlohmann/json | 3.12.0 | MIT; downloaded `vendor/licenses/json.LICENSE` |
| pugixml | 1.15 | MIT; downloaded `vendor/licenses/pugixml.LICENSE` |
| stb_image_write | commit 1ee679ca2ef753a528db5ba6801e1067b40481b8 | MIT/public-domain alternatives; downloaded `vendor/licenses/stb.LICENSE` and header |
| Inter | Repository asset from the Inter project | SIL Open Font License 1.1; `assets/Inter-LICENSE.txt` |
| Microsoft VC++ runtime | Installed separately by the user | Microsoft redistribution/license terms; not included in source packages |

Exact dependency download URLs/hashes are in `dependencies.lock.json`. Dependency archives and generated binaries are not included in the source-release ZIP. Users download official artifacts during setup. GCC/libstdc++ runtime exceptions and MinGW runtime notices apply to the relevant generated binary components; review upstream notices before introducing a prebuilt binary bundle.

The Inter font is included unmodified, with its original license. Football-Data, ESPN, API-Football and RSS/news providers are data sources, not project code dependencies. Their data are fetched at runtime, are not licensed by this project's GPL, and are not bundled in releases.
