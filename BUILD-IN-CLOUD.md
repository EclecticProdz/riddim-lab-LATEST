# Build Riddim Lab in the cloud (no local Xcode install)

This project includes a GitHub Actions workflow that builds the macOS VST3 and Audio Unit on a hosted macOS runner.

1. Sign in to GitHub in your browser and create a **new repository**. For easiest setup, choose **Public** and do not initialize it with a README/license/gitignore.
2. On the new repository page, choose **Add file → Upload files** and upload the *contents* of this project folder (including the hidden `.github` folder). If your browser hides `.github`, use the GitHub web editor to create `.github/workflows/build-macos.yml` from the included file.
3. Commit the upload to `main`. Open the repository's **Actions** tab and select **Build Riddim Lab for macOS**.
4. When the run finishes successfully, open the run and download the artifact named `Dub-Chamber-macOS-plugins`.
5. Unzip it. Copy `Riddim Lab.vst3` into `~/Library/Audio/Plug-Ins/VST3/`. For the Audio Unit, copy `Riddim Lab.component` into `~/Library/Audio/Plug-Ins/Components/`. Create those folders if needed, then restart Ableton Live and rescan plug-ins.

Notes:
- This is an unsigned development build. macOS security or Ableton may reject it; the build succeeding does not yet prove it has been tested in Ableton.
- If the build fails, open the failed workflow run and share the first meaningful compiler/configuration error.
- Do not upload private credentials or unrelated files to the repository.
