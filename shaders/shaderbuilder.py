from pathlib import Path
import subprocess
import re
import shutil

re_shader_pattern = re.compile(r'\[shader\("(\w+)"\)\]', re.IGNORECASE)
shader_entry_points = {
    "vertex": "vsMain",
    "pixel": "psMain",
    "compute": "csMain"
}

def build_shaders():
    out_msg = {}

    root_dir = Path(__file__).resolve().parent.parent.parent.resolve()
    out_msg["root_dir"] = root_dir.name

    # TODO: Need to have all slangc binaries and libs in root folder where
    #       fireplateau is running. This needs to be changed. Use env or something.
    #       Or rather embed slangc in the project in a sensible manner.
    slangc = root_dir / "slangc"
    shader_src_dir = root_dir.parent / "shaders/src"
    shader_build_dir = root_dir / "shaders"
    shader_build_dir.mkdir(parents=True, exist_ok=True)

    shader_sources = shader_src_dir.glob("*.slang")
    for shader in shader_sources:
        shader_name = shader.stem
        shader_date = shader.stat().st_mtime
        shader_stages = []
        built_stages = []
        with open(shader, "r") as f:
            content = f.read()
            shader_stages = re_shader_pattern.findall(content)

        for stage in shader_stages:
            # Check if shader source and spirv dates differ (by minute accuracy)
            #if (shader_build_dir / f"{shader_name}_{stage}.spv").exists():
                #if (shader_date - (shader_build_dir / f"{shader_name}_{stage}.spv").stat().st_mtime) < 60:
                    #continue

            shader_build_path = shader_build_dir / f"{shader_name}_{stage}.spv"
            print(f"Building {shader_name} {stage} shader...")

            subprocess.run([
                str(slangc), str(shader),
                "-entry", shader_entry_points[stage],
                "-stage", stage,
                "-target", "spirv",
                "-o", str(shader_build_path)], check=True)
            built_stages.append(stage)
        out_msg[shader_name] = sorted(set(built_stages))
    return out_msg

if __name__ == "__main__":
    build_shaders()