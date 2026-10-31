"""Combine a body STL and its text-inlay STL into one PrusaSlicer 3MF.

The result is a single object with two parts: the body on extruder 1 and
the text on extruder 2, in their original relative positions. Open it in
PrusaSlicer, pick your printer (XL 2T, or MMU) and filaments, and slice.

usage: python make_3mf.py out.3mf NAME body.stl text.stl [body_extruder text_extruder]
needs: pip install trimesh numpy
"""
import sys
import zipfile
from xml.sax.saxutils import escape

import numpy as np
import trimesh


def load(path):
    m = trimesh.load(path, force="mesh")
    m.merge_vertices()
    return m


def main():
    out, name, body_p, text_p = sys.argv[1:5]
    ext = [int(x) for x in sys.argv[5:7]] if len(sys.argv) >= 7 else [1, 2]
    parts = [("body", load(body_p), ext[0]), ("text", load(text_p), ext[1])]

    verts, tris, ranges, voff, toff = [], [], [], 0, 0
    for pname, m, e in parts:
        verts.append(m.vertices)
        tris.append(m.faces + voff)
        ranges.append((pname, toff, toff + len(m.faces) - 1, e))
        voff += len(m.vertices)
        toff += len(m.faces)
    V = np.vstack(verts)
    T = np.vstack(tris)

    vxml = "".join(f'<vertex x="{x:.5f}" y="{y:.5f}" z="{z:.5f}"/>' for x, y, z in V)
    txml = "".join(f'<triangle v1="{a}" v2="{b}" v3="{c}"/>' for a, b, c in T)
    model = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<model unit="millimeter" xml:lang="en-US" '
        'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02" '
        'xmlns:slic3rpe="http://schemas.slic3r.org/3mf/2017/06">\n'
        '<metadata name="slic3rpe:Version3mf">1</metadata>\n'
        f'<metadata name="Title">{escape(name)}</metadata>\n'
        '<resources>\n'
        f'<object id="1" type="model"><mesh><vertices>{vxml}</vertices>'
        f'<triangles>{txml}</triangles></mesh></object>\n'
        '</resources>\n'
        '<build><item objectid="1" transform="1 0 0 0 1 0 0 0 1 0 0 0" printable="1"/></build>\n'
        '</model>\n'
    )

    cfg = ['<?xml version="1.0" encoding="UTF-8"?>', '<config>',
           '<object id="1" instances_count="1">',
           f'<metadata type="object" key="name" value="{escape(name)}"/>']
    for pname, first, last, e in ranges:
        cfg += [f'<volume firstid="{first}" lastid="{last}">',
                f'<metadata type="volume" key="name" value="{escape(name)} {pname}"/>',
                '<metadata type="volume" key="volume_type" value="ModelPart"/>',
                f'<metadata type="volume" key="extruder" value="{e}"/>',
                '</volume>']
    cfg += ['</object>', '</config>']

    content_types = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
        '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
        '<Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/>'
        '<Default Extension="config" ContentType="text/xml"/>'
        '</Types>')
    rels = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Target="/3D/3dmodel.model" Id="rel-1" '
        'Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/>'
        '</Relationships>')

    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", content_types)
        z.writestr("_rels/.rels", rels)
        z.writestr("3D/3dmodel.model", model)
        z.writestr("Metadata/Slic3r_PE_model.config", "\n".join(cfg))
    print(out, [(r[0], r[2] - r[1] + 1, "extruder", r[3]) for r in ranges])


if __name__ == "__main__":
    main()
