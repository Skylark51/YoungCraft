#include "io/StructureReader.h"

#include <gemmi/cif.hpp>
#include <gemmi/numb.hpp>
#include <gemmi/symmetry.hpp>
#include <gemmi/unitcell.hpp>

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace youngcraft {
namespace {

QStringList linesOf(const QByteArray &bytes) {
    return QString::fromUtf8(bytes).split(QRegularExpression(QStringLiteral("\\r?\\n")));
}

QStringList fields(const QString &line) {
    return line.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

double number(const QString &text, double fallback = 0) {
    bool ok = false;
    const double value = text.toDouble(&ok);
    return ok && std::isfinite(value) ? value : fallback;
}

int elementNumber(QString symbol, const QString &label = {}) {
    symbol = symbol.trimmed();
    if (symbol.isEmpty() || symbol == QStringLiteral(".") || symbol == QStringLiteral("?"))
        symbol = label;
    const QRegularExpressionMatch match = QRegularExpression(QStringLiteral("^([A-Za-z]{1,2})")).match(symbol);
    if (!match.hasMatch()) return 0;
    const QString letters = match.captured(1);
    if (letters.size() == 2) {
        const int two = atomicNumberFor(letters.left(1).toUpper() + letters.mid(1).toLower());
        if (two > 0) return two;
    }
    return atomicNumberFor(letters.left(1).toUpper());
}

Atom atomAt(int z, const QString &label, const QVector3D &position) {
    Atom atom;
    atom.atomicNumber = z;
    atom.element = elementSymbol(z);
    atom.label = label;
    atom.position = position;
    return atom;
}

void addAtom(Molecule &molecule, Atom atom) {
    atom.index = molecule.atoms.size() + 1;
    molecule.atoms.push_back(atom);
}

gemmi::Position gemmiPosition(const QVector3D &p) { return {p.x(), p.y(), p.z()}; }
QVector3D vector3(const gemmi::Position &p) { return {float(p.x), float(p.y), float(p.z)}; }

std::shared_ptr<CrystalData> makeCrystal(const gemmi::UnitCell &cell, const QString &spaceGroup) {
    if (!cell.is_crystal()) return {};
    auto crystal = std::make_shared<CrystalData>();
    crystal->a = cell.a; crystal->b = cell.b; crystal->c = cell.c;
    crystal->alpha = cell.alpha; crystal->beta = cell.beta; crystal->gamma = cell.gamma;
    crystal->axisA = vector3(cell.orthogonalize({1, 0, 0}));
    crystal->axisB = vector3(cell.orthogonalize({0, 1, 0}));
    crystal->axisC = vector3(cell.orthogonalize({0, 0, 1}));
    crystal->spaceGroup = spaceGroup;
    return crystal;
}

Molecule makePacking(const Molecule &cell, const CrystalData &crystal) {
    Molecule packed;
    packed.title = cell.title;
    for (int x = 0; x < 2; ++x)
        for (int y = 0; y < 2; ++y)
            for (int z = 0; z < 2; ++z) {
                const int offset = packed.atoms.size();
                const QVector3D shift = x * crystal.axisA + y * crystal.axisB + z * crystal.axisC;
                for (Atom atom : cell.atoms) { atom.position += shift; addAtom(packed, atom); }
                for (Bond bond : cell.bonds) {
                    bond.first += offset; bond.second += offset;
                    packed.bonds.push_back(bond);
                }
            }
    return packed;
}

void addPackingBoundaryBonds(Molecule &packed, const Molecule &unit, const CrystalData &crystal) {
    const int count = unit.atoms.size();
    if (!count) return;
    const auto copyIndex = [](int x, int y, int z) { return (x * 2 + y) * 2 + z; };
    for (int i = 0; i < count; ++i)
        for (int j = 0; j < count; ++j) {
            const Atom &first = unit.atoms[i], &second = unit.atoms[j];
            if (first.atomicNumber == 1 && second.atomicNumber == 1) continue;
            const bool metal = isMetal(first.atomicNumber) || isMetal(second.atomicNumber);
            const double cutoff = (covalentRadius(first.atomicNumber) +
                                   covalentRadius(second.atomicNumber)) * (metal ? 1.45 : 1.25);
            for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dz = -1; dz <= 1; ++dz) {
                        if (dx == 0 && dy == 0 && dz == 0) continue;
                        const QVector3D delta = second.position - first.position +
                                                dx * crystal.axisA + dy * crystal.axisB +
                                                dz * crystal.axisC;
                        const double distance = delta.length();
                        if (distance <= 0.4 || distance > cutoff) continue;
                        for (int x = 0; x < 2; ++x)
                            for (int y = 0; y < 2; ++y)
                                for (int z = 0; z < 2; ++z) {
                                    const int tx = x + dx, ty = y + dy, tz = z + dz;
                                    if (tx < 0 || tx > 1 || ty < 0 || ty > 1 || tz < 0 || tz > 1) continue;
                                    const int a = copyIndex(x, y, z) * count + i;
                                    const int b = copyIndex(tx, ty, tz) * count + j;
                                    if (a < b)
                                        packed.bonds.push_back({a, b, 1.0, metal,
                                                                metal ? BondStyle::Dashed : BondStyle::Solid, false});
                                }
                    }
        }
}

void finish(StructureLoad &load, const QString &path, const QString &format) {
    auto &result = load.result;
    result.data.filePath = path;
    result.data.format = format;
    if (result.data.molecule.atoms.isEmpty()) {
        result.error = QStringLiteral("원자 좌표를 찾지 못했습니다.");
        return;
    }
    if (result.data.molecule.bonds.isEmpty() && !load.crystal)
        result.data.molecule.rebuildBonds();
    Frame frame;
    frame.metrics.step = 1;
    for (const Atom &atom : result.data.molecule.atoms) frame.coordinates.push_back(atom.position);
    result.data.frames.push_back(frame);
    result.success = true;
}

gemmi::cif::Column column(gemmi::cif::Block &block, std::initializer_list<const char *> tags) {
    for (const char *tag : tags) {
        auto value = block.find_values(tag);
        if (value) return value;
    }
    return {};
}

QString cellValue(gemmi::cif::Block &block, std::initializer_list<const char *> tags) {
    const auto value = column(block, tags);
    return value && value.length() ? QString::fromStdString(gemmi::cif::as_string(value.at(0))) : QString();
}

QString columnText(const gemmi::cif::Column &column, int row) {
    return column && row < column.length() ? QString::fromStdString(gemmi::cif::as_string(column.at(row))) : QString();
}

double cifNumber(const QString &value, double fallback = 0) {
    if (value.isEmpty() || value == QStringLiteral(".") || value == QStringLiteral("?")) return fallback;
    return gemmi::cif::as_number(value.toStdString(), fallback);
}

double bondOrder(const QString &text) {
    const QString value = text.trimmed().toUpper();
    if (value == QStringLiteral("AR") || value == QStringLiteral("ARO") || value == QStringLiteral("A")) return 1.5;
    if (value == QStringLiteral("D") || value == QStringLiteral("DOUB") || value == QStringLiteral("2")) return 2;
    if (value == QStringLiteral("T") || value == QStringLiteral("TRIP") || value == QStringLiteral("3")) return 3;
    return 1;
}

void perceiveCrystalBonds(Molecule &molecule, const gemmi::UnitCell &cell) {
    // Only draw bonds represented by a short segment in this displayed cell.
    // The periodic minimum image decides connectivity; long boundary-spanning
    // segments are represented by neighboring copies in the packing view.
    for (int i = 0; i < molecule.atoms.size(); ++i)
        for (int j = i + 1; j < molecule.atoms.size(); ++j) {
            const Atom &a = molecule.atoms[i], &b = molecule.atoms[j];
            if (a.atomicNumber == 1 && b.atomicNumber == 1) continue;
            const double cutoff = (covalentRadius(a.atomicNumber) + covalentRadius(b.atomicNumber)) *
                                  (isMetal(a.atomicNumber) || isMetal(b.atomicNumber) ? 1.45 : 1.25);
            const double periodic = std::sqrt(cell.find_nearest_pbc_image(gemmiPosition(a.position),
                                                                           gemmiPosition(b.position)).dist_sq);
            const double direct = (a.position - b.position).length();
            if (periodic > 0.4 && periodic <= cutoff && direct <= cutoff + 0.02)
                molecule.setBond(i, j, 1.0, isMetal(a.atomicNumber) || isMetal(b.atomicNumber));
        }
}

StructureLoad readCif(const QByteArray &bytes) {
    StructureLoad load;
    auto document = gemmi::cif::read_memory(bytes.constData(), size_t(bytes.size()), "Youngcraft CIF");
    for (auto &block : document.blocks) {
        const auto labels = column(block, {"_atom_site_label", "_atom_site.label_atom_id", "_atom_site.auth_atom_id"});
        const auto symbols = column(block, {"_atom_site_type_symbol", "_atom_site.type_symbol"});
        const auto fx = column(block, {"_atom_site_fract_x", "_atom_site.fract_x"});
        const auto fy = column(block, {"_atom_site_fract_y", "_atom_site.fract_y"});
        const auto fz = column(block, {"_atom_site_fract_z", "_atom_site.fract_z"});
        const auto cx = column(block, {"_atom_site_Cartn_x", "_atom_site.Cartn_x"});
        const auto cy = column(block, {"_atom_site_Cartn_y", "_atom_site.Cartn_y"});
        const auto cz = column(block, {"_atom_site_Cartn_z", "_atom_site.Cartn_z"});
        if ((!fx || !fy || !fz) && (!cx || !cy || !cz)) continue;
        const auto occ = column(block, {"_atom_site_occupancy", "_atom_site.occupancy"});
        const auto disorder = column(block, {"_atom_site_disorder_group", "_atom_site.disorder_group",
                                              "_atom_site.label_alt_id"});
        const int rows = fx ? fx.length() : cx.length();
        const double a = cifNumber(cellValue(block, {"_cell_length_a", "_cell.length_a"}));
        const double b = cifNumber(cellValue(block, {"_cell_length_b", "_cell.length_b"}));
        const double c = cifNumber(cellValue(block, {"_cell_length_c", "_cell.length_c"}));
        gemmi::UnitCell cell;
        if (a > 0 && b > 0 && c > 0)
            cell.set(a, b, c,
                     cifNumber(cellValue(block, {"_cell_angle_alpha", "_cell.angle_alpha"}), 90),
                     cifNumber(cellValue(block, {"_cell_angle_beta", "_cell.angle_beta"}), 90),
                     cifNumber(cellValue(block, {"_cell_angle_gamma", "_cell.angle_gamma"}), 90));
        if (fx && !cell.is_crystal()) throw std::runtime_error("fractional CIF coordinates require a valid unit cell");
        const QString sgName = cellValue(block, {"_space_group_name_H-M_alt", "_symmetry_space_group_name_H-M",
                                                 "_space_group.name_H-M_alt", "_symmetry.space_group_name_H-M"});
        const int sgNumber = int(cifNumber(cellValue(block, {"_space_group_IT_number", "_symmetry_Int_Tables_number",
                                                               "_space_group.IT_number"})));
        load.crystal = makeCrystal(cell, sgName);
        Molecule asym;
        asym.title = QString::fromStdString(block.name);
        QVector<gemmi::Fractional> fractional;
        for (int row = 0; row < rows; ++row) {
            const QString label = columnText(labels, row);
            const int z = elementNumber(columnText(symbols, row), label);
            if (z <= 0) throw std::runtime_error("CIF atom has an unknown element");
            gemmi::Fractional f;
            QVector3D position;
            if (fx) {
                f = {cifNumber(columnText(fx, row)), cifNumber(columnText(fy, row)),
                     cifNumber(columnText(fz, row))};
                position = vector3(cell.orthogonalize(f));
            } else {
                position = {float(cifNumber(columnText(cx, row))), float(cifNumber(columnText(cy, row))),
                            float(cifNumber(columnText(cz, row)))};
                if (cell.is_crystal()) f = cell.fractionalize(gemmiPosition(position));
            }
            Atom atom = atomAt(z, label, position);
            atom.occupancy = cifNumber(columnText(occ, row), 1);
            atom.disorderGroup = columnText(disorder, row);
            addAtom(asym, atom);
            fractional.push_back(f);
        }
        const auto bondA = column(block, {"_geom_bond_atom_site_label_1", "_geom_bond.atom_site_label_1"});
        const auto bondB = column(block, {"_geom_bond_atom_site_label_2", "_geom_bond.atom_site_label_2"});
        const auto bondType = column(block, {"_geom_bond_type", "_geom_bond.type"});
        if (bondA && bondB) {
            QHash<QString, int> byLabel;
            for (int i = 0; i < asym.atoms.size(); ++i) byLabel.insert(asym.atoms[i].label, i);
            for (int i = 0; i < qMin(bondA.length(), bondB.length()); ++i) {
                const QString first = columnText(bondA, i), second = columnText(bondB, i);
                if (byLabel.contains(first) && byLabel.contains(second))
                    asym.setBond(byLabel.value(first), byLabel.value(second), bondOrder(columnText(bondType, i)));
            }
        }
        if (!load.crystal) { load.result.data.molecule = asym; return load; }
        load.crystal->asymmetricUnit = asym;
        std::vector<gemmi::Op> operations;
        const auto symops = column(block, {"_space_group_symop_operation_xyz", "_symmetry_equiv_pos_as_xyz",
                                            "_space_group_symop.operation_xyz", "_symmetry_equiv.pos_as_xyz"});
        if (symops) {
            for (const std::string &value : symops)
                operations.push_back(gemmi::parse_triplet(gemmi::cif::as_string(value)));
        } else {
            const gemmi::SpaceGroup *group = nullptr;
            if (!sgName.isEmpty()) group = gemmi::find_spacegroup_by_name(sgName.toStdString());
            if (!group && sgNumber > 0) group = gemmi::find_spacegroup_by_number(sgNumber);
            if (group) operations = group->operations().all_ops_sorted();
        }
        if (operations.empty()) operations.push_back(gemmi::Op::identity());
        else if (std::none_of(operations.begin(), operations.end(),
                              [](const gemmi::Op &op) { return op == gemmi::Op::identity(); }))
            operations.insert(operations.begin(), gemmi::Op::identity());
        if (load.crystal->spaceGroup.isEmpty() && sgNumber > 0)
            load.crystal->spaceGroup = QString::number(sgNumber);
        Molecule unit;
        unit.title = asym.title;
        QVector<QPair<int, int>> origins;
        for (int opIndex = 0; opIndex < int(operations.size()); ++opIndex)
            for (int site = 0; site < asym.atoms.size(); ++site) {
                const auto source = fractional[size_t(site)];
                const auto transformed = operations[size_t(opIndex)].apply_to_xyz({source.x, source.y, source.z});
                gemmi::Fractional wrapped{transformed[0] - std::floor(transformed[0]),
                                          transformed[1] - std::floor(transformed[1]),
                                          transformed[2] - std::floor(transformed[2])};
                const QVector3D p = vector3(cell.orthogonalize(wrapped));
                bool duplicate = false;
                for (int i = 0; i < unit.atoms.size(); ++i)
                    if (origins[i].second == site && (unit.atoms[i].position - p).length() < 0.05f) {
                        duplicate = true; break;
                    }
                if (duplicate) continue;
                Atom atom = asym.atoms[site]; atom.position = p;
                addAtom(unit, atom);
                origins.push_back({opIndex, site});
            }
        if (!asym.bonds.isEmpty()) {
            for (int i = 0; i < origins.size(); ++i)
                for (int j = i + 1; j < origins.size(); ++j)
                    if (origins[i].first == origins[j].first) {
                        const int index = asym.bondIndex(origins[i].second, origins[j].second);
                        if (index >= 0 && (unit.atoms[i].position - unit.atoms[j].position).length() < 3.5f) {
                            const Bond &bond = asym.bonds[index];
                            unit.setBond(i, j, bond.order, bond.coordination, bond.style, true);
                        }
                    }
        } else perceiveCrystalBonds(unit, cell);
        load.crystal->unitCell = unit;
        load.crystal->packing = makePacking(unit, *load.crystal);
        if (asym.bonds.isEmpty())
            addPackingBoundaryBonds(load.crystal->packing, unit, *load.crystal);
        load.result.data.molecule = asym;
        return load;
    }
    throw std::runtime_error("CIF atom_site loop was not found");
}

StructureLoad readMol(const QStringList &lines) {
    StructureLoad load;
    if (lines.size() < 4) throw std::runtime_error("MOL header is incomplete");
    Molecule &m = load.result.data.molecule;
    m.title = lines[0].trimmed();
    const QString counts = lines[3];
    if (counts.contains(QStringLiteral("V3000"))) {
        bool atoms = false, bonds = false;
        for (const QString &line : lines) {
            if (line.contains(QStringLiteral("BEGIN ATOM"))) { atoms = true; continue; }
            if (line.contains(QStringLiteral("END ATOM"))) { atoms = false; continue; }
            if (line.contains(QStringLiteral("BEGIN BOND"))) { bonds = true; continue; }
            if (line.contains(QStringLiteral("END BOND"))) { bonds = false; continue; }
            if (!line.startsWith(QStringLiteral("M  V30 "))) continue;
            const QStringList parts = fields(line.mid(7));
            if (atoms && parts.size() >= 5) {
                const int z = elementNumber(parts[1]);
                if (z > 0) addAtom(m, atomAt(z, parts[1], {float(number(parts[2])), float(number(parts[3])), float(number(parts[4]))}));
            } else if (bonds && parts.size() >= 4) m.setBond(parts[2].toInt() - 1, parts[3].toInt() - 1, bondOrder(parts[1]));
        }
    } else {
        const int atomCount = counts.left(3).trimmed().toInt();
        const int bondCount = counts.mid(3, 3).trimmed().toInt();
        if (atomCount < 1 || lines.size() < 4 + atomCount + bondCount) throw std::runtime_error("MOL counts are invalid");
        for (int i = 0; i < atomCount; ++i) {
            const QString line = lines[4 + i];
            const int z = elementNumber(line.mid(31, 3));
            if (z <= 0) throw std::runtime_error("MOL atom element is invalid");
            addAtom(m, atomAt(z, elementSymbol(z), {float(number(line.mid(0, 10))),
                                                   float(number(line.mid(10, 10))), float(number(line.mid(20, 10)))}));
        }
        for (int i = 0; i < bondCount; ++i) {
            const QString line = lines[4 + atomCount + i];
            m.setBond(line.mid(0, 3).toInt() - 1, line.mid(3, 3).toInt() - 1,
                      bondOrder(line.mid(6, 3)));
        }
    }
    return load;
}

StructureLoad readMol2(const QStringList &lines) {
    StructureLoad load;
    Molecule &m = load.result.data.molecule;
    QHash<int, int> indices;
    QString section;
    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("@<TRIPOS>"))) { section = line.mid(9).trimmed(); continue; }
        const QStringList parts = fields(line);
        if (section == QStringLiteral("ATOM") && parts.size() >= 6) {
            const QString symbol = parts[5].section(QLatin1Char('.'), 0, 0);
            const int z = elementNumber(symbol, parts[1]);
            if (z <= 0) throw std::runtime_error("MOL2 atom element is invalid");
            indices.insert(parts[0].toInt(), m.atoms.size());
            addAtom(m, atomAt(z, parts[1], {float(number(parts[2])), float(number(parts[3])), float(number(parts[4]))}));
        } else if (section == QStringLiteral("BOND") && parts.size() >= 4 &&
                   indices.contains(parts[1].toInt()) && indices.contains(parts[2].toInt())) {
            m.setBond(indices.value(parts[1].toInt()), indices.value(parts[2].toInt()), bondOrder(parts[3]));
        }
    }
    return load;
}

StructureLoad readPdb(const QStringList &lines) {
    StructureLoad load;
    Molecule &m = load.result.data.molecule;
    QHash<int, int> serials;
    for (const QString &line : lines) {
        const QString record = line.left(6).trimmed();
        if (record == QStringLiteral("ENDMDL")) break;
        if (record != QStringLiteral("ATOM") && record != QStringLiteral("HETATM")) continue;
        const QString alt = line.mid(16, 1).trimmed();
        if (!alt.isEmpty() && alt != QStringLiteral("A")) continue;
        const QString label = line.mid(12, 4).trimmed();
        QString symbol = line.mid(76, 2).trimmed();
        if (symbol.isEmpty()) symbol = label.left(1); // PDB CA is usually carbon alpha.
        const int z = elementNumber(symbol);
        if (z <= 0) continue;
        serials.insert(line.mid(6, 5).toInt(), m.atoms.size());
        Atom atom = atomAt(z, label, {float(number(line.mid(30, 8))), float(number(line.mid(38, 8))),
                                       float(number(line.mid(46, 8)))});
        atom.occupancy = number(line.mid(54, 6), 1);
        atom.disorderGroup = alt;
        addAtom(m, atom);
    }
    for (const QString &line : lines) {
        if (!line.startsWith(QStringLiteral("CONECT"))) continue;
        const int first = line.mid(6, 5).toInt();
        if (!serials.contains(first)) continue;
        for (int i = 11; i + 5 <= line.size(); i += 5) {
            const int other = line.mid(i, 5).toInt();
            if (serials.contains(other)) m.setBond(serials.value(first), serials.value(other), 1);
        }
    }
    return load;
}

StructureLoad readShelx(const QStringList &lines) {
    StructureLoad load;
    Molecule &m = load.result.data.molecule;
    gemmi::UnitCell cell;
    QStringList sfac;
    for (const QString &line : lines) {
        const QStringList parts = fields(line);
        if (parts.isEmpty()) continue;
        const QString key = parts[0].toUpper();
        if (key == QStringLiteral("CELL") && parts.size() >= 8)
            cell.set(number(parts[2]), number(parts[3]), number(parts[4]),
                     number(parts[5]), number(parts[6]), number(parts[7]));
        else if (key == QStringLiteral("SFAC") && parts.size() > 1) {
            for (int i = 1; i < parts.size(); ++i) if (elementNumber(parts[i]) > 0) sfac.push_back(parts[i]);
        } else if (cell.is_crystal() && parts.size() >= 5 &&
                   parts[1].toInt() > 0 && parts[1].toInt() <= sfac.size()) {
            const int z = elementNumber(sfac[parts[1].toInt() - 1]);
            if (z <= 0) continue;
            const gemmi::Fractional f{number(parts[2]), number(parts[3]), number(parts[4])};
            Atom atom = atomAt(z, parts[0], vector3(cell.orthogonalize(f)));
            if (parts.size() > 5) atom.occupancy = std::fmod(std::abs(number(parts[5])), 10.0);
            addAtom(m, atom);
        }
    }
    load.crystal = makeCrystal(cell, {});
    if (load.crystal) {
        perceiveCrystalBonds(m, cell);
        load.crystal->asymmetricUnit = m;
        load.crystal->unitCell = m;
        load.crystal->packing = makePacking(m, *load.crystal);
        addPackingBoundaryBonds(load.crystal->packing, m, *load.crystal);
    }
    return load;
}

StructureLoad readCssr(const QStringList &lines) {
    StructureLoad load;
    if (lines.size() < 5) throw std::runtime_error("CSSR header is incomplete");
    const QStringList lengths = fields(lines[0]), angles = fields(lines[1]);
    if (lengths.size() < 3 || angles.size() < 3) throw std::runtime_error("CSSR cell is invalid");
    gemmi::UnitCell cell(number(lengths[0]), number(lengths[1]), number(lengths[2]),
                         number(angles[0]), number(angles[1]), number(angles[2]));
    if (!cell.is_crystal()) throw std::runtime_error("CSSR cell is invalid");
    Molecule &m = load.result.data.molecule;
    const int count = fields(lines[2]).value(0).toInt();
    for (int i = 4; i < lines.size() && m.atoms.size() < count; ++i) {
        const QStringList parts = fields(lines[i]);
        if (parts.size() < 5) continue;
        bool indexOk = false;
        parts[0].toInt(&indexOk);
        if (!indexOk) continue;
        const int z = elementNumber(parts[1]);
        if (z <= 0) continue;
        const gemmi::Fractional f{number(parts[2]), number(parts[3]), number(parts[4])};
        addAtom(m, atomAt(z, parts[1], vector3(cell.orthogonalize(f))));
    }
    load.crystal = makeCrystal(cell, {});
    perceiveCrystalBonds(m, cell);
    load.crystal->asymmetricUnit = m;
    load.crystal->unitCell = m;
    load.crystal->packing = makePacking(m, *load.crystal);
    addPackingBoundaryBonds(load.crystal->packing, m, *load.crystal);
    return load;
}

} // namespace

StructureLoad StructureReader::read(const QString &path, const QString &format) {
    StructureLoad load;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        load.result.error = file.errorString();
        return load;
    }
    const QByteArray bytes = file.readAll();
    try {
        if (format == QStringLiteral("CIF")) load = readCif(bytes);
        else if (format == QStringLiteral("MOL") || format == QStringLiteral("SDF")) load = readMol(linesOf(bytes));
        else if (format == QStringLiteral("MOL2")) load = readMol2(linesOf(bytes));
        else if (format == QStringLiteral("PDB")) load = readPdb(linesOf(bytes));
        else if (format == QStringLiteral("SHELX")) load = readShelx(linesOf(bytes));
        else if (format == QStringLiteral("CSSR")) load = readCssr(linesOf(bytes));
        else throw std::runtime_error("Unsupported structure format");
        finish(load, path, format);
    } catch (const std::exception &error) {
        load.result.success = false;
        load.result.error = QString::fromUtf8(error.what());
    }
    return load;
}

} // namespace youngcraft
