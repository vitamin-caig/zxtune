# Commentaries

Comments should explain WHY code exists rather than what it does.

Good examples:

```c++
// Some of the modules (e.g. Story Map.psc) references more ornaments than really stored
const std::size_t maxOrnaments = (ornamentsTableEnd - ornamentsTableStart) / sizeof(uint16_t);
```

```c++
case 3:
  // copyright/publisher really
  props.SetComment(Strings::SanitizeMultiline(tuneInfo.infoString(2)));
```

```c++
// If looped, do not allow fadein
// to avoid absolute silence
const auto part1 = doneLoops ? Preamp : Fading.GetFadein(Preamp, posAfter);
```

```c++
// do not add/add to 'success' error
if (e && *this)
```

```c++
// selection->currentIndex() does not work
// items are orderen by selection, not by position
QModelIndexList items = selection->selectedRows();
```

Bad examples:

```c++
// add text to set
auto text = templ.substr(textBegin, fieldBegin - textBegin);
FixedStrings.emplace_back(text);
```

```c++
// check if required to revert table
const bool doRevert = !id.empty() && *id.begin() == REVERT_TABLE_MARK;
```
