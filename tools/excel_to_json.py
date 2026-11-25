#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Convertit un classeur Excel EBIOS RM (gabarit à 22 feuilles) en projet .ebios (JSON) pour EBIOS RM Studio.

Usage : python3 excel_to_json.py classeur.xlsx sortie.ebios [annexe.json]

annexe.json (facultatif) complète ce que le classeur ne contient que sous forme d'images ou de textes libres :
  { "ss_er": {"SS-01": "ER-02", ...},          # événements redoutés de chaque scénario stratégique
    "ss_ov": {"SS-01": "OV-01", ...},          # objectif visé de chaque scénario stratégique
    "actions": {"SO1": [["Connaître", "texte", 4], ...]},   # actions élémentaires (phase, texte, vraisemblance 1-4)
    "titre": "...", "architecture_description": "...",
    "architecture_image": "xl/media/imageN.png" }  # image du classeur à reprendre comme schéma d'architecture
Sans annexe, ces éléments restent vides et se saisissent dans l'application.
"""
import sys, json, base64, re, datetime, zipfile, warnings
warnings.filterwarnings("ignore")
from openpyxl import load_workbook
from openpyxl.utils import column_index_from_string

src, dst = sys.argv[1], sys.argv[2]
wb = load_workbook(src, data_only=True)

extra = {}
if len(sys.argv) > 3:       # fichier annexe facultatif (voir README) : liaisons SS -> ER / OV et actions élémentaires des SO
    with open(sys.argv[3], encoding="utf-8") as fx:
        extra = json.load(fx)


def s(v):
    if v is None:
        return ""
    if isinstance(v, datetime.datetime):
        return v.strftime("%Y-%m-%d")
    if isinstance(v, float) and v.is_integer():
        return str(int(v))
    return str(v).strip()


def rows(name, first):
    ws = wb[name]
    for r in ws.iter_rows(min_row=first, max_row=ws.max_row):
        yield r


def cell(r, col):
    idx = column_index_from_string(col) - 1
    return r[idx].value if idx < len(r) else None


T = {}
ELEV = {"Elevée": "Élevée", "Elevé": "Élevé"}


def fix(v):
    v = s(v)
    return ELEV.get(v, v)


# --- Sommaire
ws = wb["Sommaire"]
sommaire = {
    "titre": "Analyse de risques EBIOS RM",
    "version": s(ws["C5"].value), "date": s(ws["C7"].value), "auteur": s(ws["C9"].value),
    "objectif": s(ws["C11"].value), "documents": s(ws["C13"].value),
    "historique": s(ws["C15"].value), "responsable": s(ws["C17"].value),
}

# --- Valeurs métiers
vm = []
for r in rows("Valeurs Métiers", 7):
    id_ = s(cell(r, "B"))
    bsid = s(cell(r, "I"))
    if id_:
        vm.append({"id": id_, "famille": s(cell(r, "A")), "titre": s(cell(r, "C")), "description": s(cell(r, "D")),
                   "d": bool(s(cell(r, "E"))), "i": bool(s(cell(r, "F"))), "c": bool(s(cell(r, "G"))),
                   "t": bool(s(cell(r, "H"))), "bs": bsid})
    elif bsid and vm:
        vm[-1]["bs"] = (vm[-1]["bs"] + "," + bsid) if vm[-1]["bs"] else bsid
T["vm"] = vm

# --- Biens supports
bs = []
for r in rows("Biens Supports", 5):
    id_ = s(cell(r, "A"))
    if re.match(r"BS-\d+", id_):
        bs.append({"id": id_, "nom": s(cell(r, "B")), "type": s(cell(r, "C")), "description": s(cell(r, "D")),
                   "d": bool(s(cell(r, "E"))), "i": bool(s(cell(r, "F"))), "c": bool(s(cell(r, "G"))),
                   "t": bool(s(cell(r, "H")))})
T["bs"] = bs

# --- Évènements redoutés
er = []
cur_vm = ""
for r in rows("Evènements Redoutés", 7):
    if s(cell(r, "A")):
        cur_vm = s(cell(r, "A"))
    id_ = s(cell(r, "C"))
    if id_:
        er.append({"id": id_, "vm": cur_vm, "titre": s(cell(r, "D")), "impacts": s(cell(r, "E")),
                   "niveau": s(cell(r, "F"))})
T["er"] = er

# --- Socle de sécurité
socle = []
for r in rows("Socle de Sécurité", 7):
    id_ = s(cell(r, "A"))
    if id_:
        conf = s(cell(r, "I"))
        if conf.startswith("Conformité prévue"):
            conf = "Conformité prévue"
        socle.append({"id": id_, "nom": s(cell(r, "B")), "regle": s(cell(r, "C")),
                      "obligatoire": s(cell(r, "D")).lower() in ("oui", "x", "true"),
                      "contenu": s(cell(r, "E")), "porteur": s(cell(r, "F")), "priorite": s(cell(r, "G")),
                      "docs": s(cell(r, "H")), "conformite": conf, "cible": s(cell(r, "J")),
                      "echeance": s(cell(r, "K")), "preuves": s(cell(r, "L")), "commentaire": s(cell(r, "M"))})
T["socle"] = socle
T["vuln"] = []

# --- Architecture (image intégrée)
arch = {"description": "", "image": ""}
img_name = extra.get("architecture_image")
if img_name:
    with zipfile.ZipFile(src) as z:
        arch["image"] = base64.b64encode(z.read(img_name)).decode()

# --- Sources de risque / Objectifs visés
def simple(name, prefix):
    out = []
    for r in rows(name, 5):
        id_ = s(cell(r, "A"))
        if id_.startswith(prefix):
            out.append({"id": id_, "nom": s(cell(r, "B")), "description": s(cell(r, "C")),
                        "retenue": s(cell(r, "D")).lower() == "oui", "justification": s(cell(r, "E"))})
    return out


sr = simple("Sources de Risque", "SR-")
T["sr"] = sr
T["ov"] = simple("Objectifs Visés", "OV-")

# --- Couples SR/OV
couples = []
for r in rows("SR-OV", 5):
    id_ = s(cell(r, "A"))
    if id_.startswith("SR.OV"):
        couples.append({"id": id_, "sr": s(cell(r, "B")), "ov": s(cell(r, "D")), "objectif": s(cell(r, "E")),
                        "motivation": fix(cell(r, "F")), "ressources": fix(cell(r, "G")),
                        "retenu": s(cell(r, "I")).lower() == "oui", "justification": s(cell(r, "J"))})
T["couples"] = couples

# --- Parties prenantes (+ projection)
pp = []
cat = ""
for r in rows("Parties Prenantes", 8):
    id_ = s(cell(r, "B"))
    if not id_.startswith("PP-"):
        continue
    if s(cell(r, "A")):
        cat = " ".join(s(cell(r, "A")).split())
    pp.append({"categorie": cat, "id": id_, "nom": s(cell(r, "C")), "mesures": s(cell(r, "D")),
               "dependance": s(cell(r, "E")), "penetration": s(cell(r, "F")),
               "maturite": s(cell(r, "H")), "confiance": s(cell(r, "I")),
               "pertinence": s(cell(r, "M")).lower() == "oui", "commentaire": s(cell(r, "N"))})
byid = {p["id"]: p for p in pp}
for r in rows("Parties Prenantes (projection)", 8):
    id_ = s(cell(r, "A"))
    if id_ in byid:
        byid[id_].update({"mesure_comp": s(cell(r, "C")), "p_dependance": s(cell(r, "D")),
                          "p_penetration": s(cell(r, "E")), "p_maturite": s(cell(r, "G")),
                          "p_confiance": s(cell(r, "H"))})
T["pp"] = pp

# --- Scénarios stratégiques (les ER / OV associés viennent de la feuille « Scénarios Stratégiques (Dét.) »)
ss_er = extra.get("ss_er", {})
ss_ov = extra.get("ss_ov", {})
srnames = sorted(sr, key=lambda x: -len(x["nom"]))
ss = []
for r in rows("Scénarios Stratégiques", 7):
    id_ = s(cell(r, "B"))
    if id_.startswith("SS-"):
        txt = s(cell(r, "C"))
        ids = [x["id"] for x in srnames if x["nom"].strip() in txt]
        ss.append({"id": id_, "sr": ",".join(sorted(ids)), "ov": ss_ov.get(id_, ""), "pp": "",
                   "titre": s(cell(r, "D")), "chemin": s(cell(r, "E")), "er": ss_er.get(id_, ""),
                   "gravite": s(cell(r, "F"))})
T["ss"] = ss

# --- Scénarios opérationnels
so = []
for r in rows("Scénarios Opérationnels", 7):
    id_ = s(cell(r, "B"))
    if id_.startswith("SO"):
        so.append({"id": id_, "ss": s(cell(r, "C")), "titre": s(cell(r, "D")), "chemin": s(cell(r, "E")),
                   "vraisemblance_manuelle": ""})
T["so"] = so

# --- Actions élémentaires : transcrites des schémas de « Scénarios Opérationnels (Dét.) »
A = {k: [tuple(x) for x in v] for k, v in extra.get("actions", {}).items()}
actions = []
n = 0
for so_id, lst in A.items():
    for ph, txt, v in lst:
        n += 1
        actions.append({"id": "AC-%02d" % n, "so": so_id, "phase": ph, "action": txt,
                        "vraisemblance": str(v), "bs": ""})
T["actions"] = actions

# --- Scénarios de risque
mc_res = {}
ws = wb["Mes. de Sec. Complémentaires"]
for col in "EFGH":
    rid = s(ws[col + "6"].value)
    if rid:
        mc_res[rid] = {"v_res": s(ws[col + "8"].value), "g_res": s(ws[col + "11"].value)}
risques = []
for r in rows("Scénarios de Risque", 7):
    id_ = s(cell(r, "B"))
    if re.match(r"R\d+$", id_):
        d = {"id": id_, "so": s(cell(r, "D")), "titre": s(cell(r, "E")), "niveau_retenu": fix(cell(r, "J")),
             "acceptabilite": s(cell(r, "K")), "decision": s(cell(r, "L")),
             "g_res": "", "v_res": "", "decision_res": "", "mesures": ""}
        d.update(mc_res.get(id_, {}))
        risques.append(d)
T["risques"] = risques


# --- Mesures complémentaires
def statut(o):
    o = o.lower()
    if o.startswith("annul"):
        return "Annulé"
    if o.startswith("fait"):
        return "Fait"
    if "en cour" in o:
        return "En cours"
    if o.startswith("a vérifier"):
        return "À vérifier"
    return "À débuter" if o else ""


mc = []
for r in rows("Mes. de Sec. Complémentaires", 14):
    id_ = s(cell(r, "B"))
    if id_.startswith("MC-"):
        o = s(cell(r, "O"))
        com = o if o and o.lower() not in ("a vérifier", "fait") else ""
        nom, desc = s(cell(r, "C")), s(cell(r, "D"))
        if not nom:
            nom, desc = desc, ""
        mc.append({"id": id_, "nom": nom, "description": desc,
                   "d": bool(s(cell(r, "E"))), "i": bool(s(cell(r, "F"))), "c": bool(s(cell(r, "G"))),
                   "t": bool(s(cell(r, "H"))), "type": s(cell(r, "I")), "perimetre": s(cell(r, "J")),
                   "responsable": s(cell(r, "K")), "limite": s(cell(r, "L")), "priorite": s(cell(r, "M")),
                   "cout": s(cell(r, "N")), "statut": statut(o), "commentaire": com, "risques": ""})
T["mc"] = mc

# --- Métriques : matrice 4x4 lue dans les couleurs de la feuille « Traitement du Risque »
F_, M_, V_ = "Faible", "Moyen", "Élevé"
matrix = [F_, F_, M_, M_,   # gravité 1 : vraisemblance 1..4
          F_, F_, M_, V_,   # gravité 2
          F_, M_, V_, V_,   # gravité 3
          M_, M_, V_, V_]   # gravité 4
metrics = {"g": ["Négligeable", "Significatif", "Important", "Catastrophique"],
           "v": ["Invraisemblable", "Moyennement vraisemblable", "Vraisemblable", "Très vraisemblable"],
           "z_hp": 0.1, "z_watch": 0.49, "z_control": 1.0, "z_danger": 3.0, "matrix": matrix}

if extra.get("titre"):
    sommaire["titre"] = extra["titre"]
if extra.get("architecture_description"):
    arch["description"] = extra["architecture_description"]

proj = {"version": 1, "sommaire": sommaire, "metrics": metrics, "architecture": arch, "tables": T}
with open(dst, "w", encoding="utf-8") as f:
    json.dump(proj, f, ensure_ascii=False, indent=1)
print("OK ->", dst, {k: len(v) for k, v in T.items()})
