/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "radio_theme.h"
#include <algorithm>

#include "bitmapbuffer.h"
#include "libopenui.h"
#include "opentx.h"
#include "file_preview.h"
#include "theme_manager.h"
#include "view_main.h"
#include "widget_settings.h"
#include "sliders.h"
#include "color_list.h"
#include "color_editor.h"
#include "listbox.h"
#include "preview_window.h"

#if (LCD_W > LCD_H)
  #define COLOR_PREVIEW_WIDTH 18
  #define COLOR_PREVIEW_HEIGHT  (LCD_H - TOPBAR_HEIGHT - 30)
  #define LEFT_LIST_WIDTH (LCD_W / 2) - COLOR_PREVIEW_WIDTH
  #define LEFT_LIST_HEIGHT (LCD_H - TOPBAR_HEIGHT - 37)
  #define COLOR_LIST_WIDTH ((LCD_W * 3)/10)
  #define COLOR_LIST_HEIGHT (LCD_H - TOPBAR_HEIGHT - 14)
  #define TOP_LIST_OFFSET 4
  #define LEFT_LIST_OFFSET 3
#else
  #define COLOR_PREVIEW_WIDTH LCD_W
  #define COLOR_PREVIEW_HEIGHT  18
  #define LEFT_LIST_WIDTH LCD_W - 6
  #define LEFT_LIST_HEIGHT (LCD_H / 2 - 38)
  #define COLOR_LIST_WIDTH LCD_W - 6
  #define COLOR_LIST_HEIGHT (LCD_H / 2 - 24)
  #define TOP_LIST_OFFSET 4
  #define LEFT_LIST_OFFSET 3
#endif

#define MARGIN_WIDTH 5

#if (LCD_W > LCD_H)
constexpr int BUTTON_HEIGHT = 30;
constexpr int BUTTON_WIDTH  = 75;
constexpr rect_t detailsDialogRect = {30, 30, LCD_W - 60, LCD_H - 60};
constexpr int labelWidth = 150;

constexpr int COLOR_BOX_LEFT = 5;
constexpr int COLOR_BOX_WIDTH = 45;
constexpr int COLOR_BOX_HEIGHT = 27;
#else
constexpr int BUTTON_HEIGHT = 30;
constexpr int BUTTON_WIDTH  = 65;
constexpr rect_t detailsDialogRect = {10, 50, LCD_W - 20, 220};
constexpr int labelWidth = 120;

constexpr int COLOR_BOX_LEFT = 3;
constexpr int COLOR_BOX_WIDTH = 55;
constexpr int COLOR_BOX_HEIGHT = 32;

constexpr int COLOR_BUTTON_WIDTH = COLOR_BOX_WIDTH;
constexpr int COLOR_BUTTON_HEIGHT = 20;
#endif

constexpr int BOX_MARGIN = 2;
constexpr int MAX_BOX_WIDTH = 15;


class ThemeColorPreview : public FormField
{
  public:
    ThemeColorPreview(Window *parent, const rect_t &rect, std::vector<ColorEntry> colorList) :
      FormField(parent, rect, NO_FOCUS),
      colorList(colorList)
    {
      setBoxWidth();
    }
    ~ThemeColorPreview()
    {
    }

    void setColorList(std::vector<ColorEntry> colorList)
    {
      this->colorList.assign(colorList.begin(), colorList.end());
      setBoxWidth();
      invalidate();
    }

    void paint(BitmapBuffer *dc) override
    {
      int totalNessessarySpace = colorList.size() * (boxWidth + 2);
      int axis = rect.w > rect.h ? rect.w : rect.h;
      axis = (axis - totalNessessarySpace) / 2;
      for (auto color: colorList) {
        if (rect.w > rect.h) {  
          dc->drawSolidFilledRect(axis, 0, boxWidth, boxWidth, COLOR2FLAGS(color.colorValue));
          dc->drawSolidRect(axis, 0, boxWidth, boxWidth, 1, COLOR2FLAGS(BLACK));
        } else {
          dc->drawSolidFilledRect(0, axis, boxWidth, boxWidth, COLOR2FLAGS(color.colorValue));
          dc->drawSolidRect(0, axis, boxWidth, boxWidth, 1, COLOR2FLAGS(BLACK));
        }
        axis += boxWidth + BOX_MARGIN;
      }
    }

  protected:
    std::vector<ColorEntry> colorList;
    int boxWidth = MAX_BOX_WIDTH;
    void setBoxWidth()
    {
      auto winSize = rect.w > rect.h ? rect.w : rect.h;
      boxWidth = winSize / (colorList.size() + BOX_MARGIN);
      boxWidth = min(boxWidth, MAX_BOX_WIDTH);
    }
};


static const lv_coord_t d_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(3),
                                       LV_GRID_TEMPLATE_LAST};
static const lv_coord_t b_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                       LV_GRID_TEMPLATE_LAST};
static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT,
                                     LV_GRID_TEMPLATE_LAST};

class ThemeDetailsDialog: public Dialog
{
  public:
    ThemeDetailsDialog(Window *parent, ThemeFile theme, std::function<void (ThemeFile theme)> saveHandler = nullptr) :
      Dialog(parent, STR_EDIT_THEME_DETAILS, detailsDialogRect),
      theme(theme),
      saveHandler(saveHandler)
    {
      lv_obj_set_style_bg_color(content->getLvObj(), makeLvColor(COLOR_THEME_SECONDARY3), 0);
      lv_obj_set_style_bg_opa(content->getLvObj(), LV_OPA_100, LV_PART_MAIN);
      auto form = new FormWindow(&content->form, rect_t{});
      form->setFlexLayout();

      FlexGridLayout grid(d_col_dsc, row_dsc, 2);

      auto line = form->newLine(&grid);

      new StaticText(line, rect_t{}, STR_NAME, 0, COLOR_THEME_PRIMARY1);
      auto te = new TextEdit(line, rect_t{}, this->theme.getName(), NAME_LENGTH);
      lv_obj_set_grid_cell(te->getLvObj(), LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

      line = form->newLine(&grid);

      new StaticText(line, rect_t{}, STR_AUTHOR, 0, COLOR_THEME_PRIMARY1);
      te = new TextEdit(line, rect_t{}, this->theme.getAuthor(), AUTHOR_LENGTH);
      lv_obj_set_grid_cell(te->getLvObj(), LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

      FlexGridLayout grid2(b_col_dsc, row_dsc, 2);

      line = form->newLine(&grid2);

      new StaticText(line, rect_t{}, STR_DESCRIPTION, 0, COLOR_THEME_PRIMARY1);
      line = form->newLine(&grid2);
      te = new TextEdit(line, rect_t{}, this->theme.getInfo(), INFO_LENGTH);
      lv_obj_set_grid_cell(te->getLvObj(), LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);

      line = form->newLine(&grid2);
      line->padTop(20);

      auto button = new TextButton(line, rect_t{0, 0, lv_pct(30), 32}, STR_SAVE, [=] () {
        if (saveHandler != nullptr)
          saveHandler(this->theme);
        deleteLater();
        return 0;
      });
      lv_obj_set_grid_cell(button->getLvObj(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

      button = new TextButton(line, rect_t{0, 0, lv_pct(30), 32}, STR_CANCEL, [=] () {
        deleteLater();
        return 0;
      });
      lv_obj_set_grid_cell(button->getLvObj(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    }

  protected:
    ThemeFile theme;
    std::function<void(ThemeFile theme)> saveHandler = nullptr;
};

class ColorSquare : public FormField
{
  public:
    ColorSquare(Window *window, const rect_t &rect, uint32_t color, WindowFlags windowFlags = 0)
      : FormField(window, rect, windowFlags | NO_FOCUS),
      _color(color)
    {
    }

    void paint (BitmapBuffer *dc) override
    {
      dc->drawSolidFilledRect(0, 0, rect.w, rect.h, COLOR2FLAGS(_color));
      dc->drawSolidRect(0, 0, rect.w, rect.h, 1, COLOR2FLAGS(BLACK));
    }

    void setColorToEdit(uint32_t colorEntry)
    {
      _color = colorEntry;

      invalidate();
    }

  protected:
    uint32_t _color;
};

void setHexStr(StaticText* hexBox, uint32_t rgb)
{
  auto r = GET_RED(rgb), g = GET_GREEN(rgb), b = GET_BLUE(rgb);
  char hexstr[8];
  snprintf(hexstr, sizeof(hexstr), "%02X%02X%02X",
           (uint16_t)r, (uint16_t)g, (uint16_t)b);
  hexBox->setText(hexstr);
}

class ColorEditPage : public Page
{
public:
  ColorEditPage(ThemeFile theme, LcdColorIndex indexOfColor, 
                std::function<void (uint32_t rgb)> setValue = nullptr) :
    Page(ICON_RADIO_EDIT_THEME),
    _setValue(std::move(setValue)),
    _indexOfColor(indexOfColor),
    _theme(theme)
  {
    buildBody(&body); 
    buildHead(&header);
  }

  void setActiveColorBar(int activeTab)
  {
    if (activeTab >= 0 && _activeTab <= (int)_tabs.size()) {
      _activeTab = activeTab;
      for (auto i = 0; i < (int) _tabs.size(); i++)
        _tabs[i]->check(i == _activeTab);
      _colorEditor->setColorEditorType(_activeTab == 1 ? HSV_COLOR_EDITOR : RGB_COLOR_EDITOR);
    }
  }

protected:
  std::function<void (uint32_t rgb)> _setValue;
  LcdColorIndex _indexOfColor;
  ThemeFile  _theme;
  TextButton *_cancelButton;
  ColorEditor *_colorEditor;
  PreviewWindow *_previewWindow = nullptr;
  std::vector<Button *> _tabs;
  int _activeTab = 0;
  ColorSquare *_colorSquare = nullptr;
  StaticText *_hexBox = nullptr;

  void buildBody(FormWindow* window)
  {
    rect_t r;

    // hexBox
    r = { COLOR_BOX_LEFT + COLOR_BOX_WIDTH + 5, TOP_LIST_OFFSET, 90, 30 };
    _hexBox = new StaticText(window, r, "", 0, COLOR_THEME_PRIMARY1 | FONT(L) | RIGHT);
    setHexStr(_hexBox, _theme.getColorEntryByIndex(_indexOfColor)->colorValue);

    if (LCD_W > LCD_H)
      r = { COLOR_BOX_LEFT, COLOR_BOX_HEIGHT + 7, COLOR_LIST_WIDTH, window->height() - COLOR_BOX_HEIGHT - 7};
    else
      r = { COLOR_BOX_LEFT, COLOR_BOX_HEIGHT + 7, COLOR_LIST_WIDTH, LCD_H / 2 - COLOR_BOX_HEIGHT - 20 };

    _colorEditor = new ColorEditor(window, r, _theme.getColorEntryByIndex(_indexOfColor)->colorValue,
      [=](uint32_t rgb) {
        _theme.setColor(_indexOfColor, rgb);
        if (_colorSquare != nullptr) {
          _colorSquare->setColorToEdit(rgb);
        }
        if (_previewWindow != nullptr) {
          _previewWindow->setColorList(_theme.getColorList());
        }
        if (_hexBox != nullptr) {
          setHexStr(_hexBox, rgb);
        }
        if (_setValue != nullptr)
          _setValue(rgb);
      });
    _colorEditor->setColorEditorType(HSV_COLOR_EDITOR);
    _activeTab = 1;

    r = { COLOR_BOX_LEFT, 4, COLOR_BOX_WIDTH, COLOR_BOX_HEIGHT };
    _colorSquare = new ColorSquare(window, r, _theme.getColorEntryByIndex(_indexOfColor)->colorValue);

    r = LCD_W > LCD_H ?
          rect_t { 
            COLOR_LIST_WIDTH + MARGIN_WIDTH + LEFT_LIST_OFFSET, 
            TOP_LIST_OFFSET, 
            LCD_W - COLOR_LIST_WIDTH - LEFT_LIST_OFFSET - MARGIN_WIDTH * 2, 
            COLOR_LIST_HEIGHT } :
          rect_t { 
            LEFT_LIST_OFFSET, 
            TOP_LIST_OFFSET + COLOR_LIST_HEIGHT + MARGIN_WIDTH,  
            LEFT_LIST_WIDTH, 
            COLOR_LIST_HEIGHT - (MARGIN_WIDTH * 2) 
          };
    _previewWindow = new PreviewWindow(window, r, _theme.getColorList());
  }

  void buildHead(PageHeader* window)
  {
    LcdFlags flags = 0;
    if (LCD_W < LCD_H) {
      flags = FONT(XS);
    }

    // page title
    new StaticText(window,
      { PAGE_TITLE_LEFT, PAGE_TITLE_TOP, LCD_W - PAGE_TITLE_LEFT,
       PAGE_LINE_HEIGHT },
      "Edit Color", 0, COLOR_THEME_PRIMARY2 | flags);
    new StaticText(window,
      { PAGE_TITLE_LEFT, PAGE_TITLE_TOP + PAGE_LINE_HEIGHT,
       LCD_W - PAGE_TITLE_LEFT, PAGE_LINE_HEIGHT },
      ThemePersistance::getColorNames()[(int)_indexOfColor], 0, COLOR_THEME_PRIMARY2 | flags);

    // page tabs
    rect_t r = { LCD_W - 2*(BUTTON_WIDTH + 5), 6, BUTTON_WIDTH, BUTTON_HEIGHT };
    _tabs.emplace_back(
      new TextButton(window, r, "RGB",
        [=] () {
          setActiveColorBar(0);
          return 1;
        }));
    r.x += (BUTTON_WIDTH + 5);
    _tabs.emplace_back(
      new TextButton(window, r, "HSV", 
        [=] () {
          setActiveColorBar(1);
          return 1;
        }));
    _tabs[1]->check(true);
  }
};

class ThemeEditPage : public Page
{
  public:
    explicit ThemeEditPage(ThemeFile theme, std::function<void (ThemeFile &theme)> saveHandler = nullptr) :
      Page(ICON_RADIO_EDIT_THEME),
      _theme(theme),
      page(this),
      saveHandler(std::move(saveHandler))
    {
      buildBody(&body);
      buildHeader(&header);
    }

    bool canCancel() override
    {
      if (_dirty) {
        new ConfirmDialog(
            this, STR_SAVE_THEME, _theme.getName(),
            [=]() {
              if (saveHandler != nullptr) {
                saveHandler(_theme);
              }
              deleteLater();
            },
            [=]() {
              deleteLater();
            });
        return false;
      }
      return true;
    }

    void editColorPage()
    {
      auto colorEntry = _cList->getSelectedColor();
      new ColorEditPage(_theme, colorEntry.colorNumber, 
      [=] (uint32_t color) {
        _dirty = true;
        _theme.setColor(colorEntry.colorNumber, color);
        _cList->setColorList(_theme.getColorList());
        _previewWindow->setColorList(_theme.getColorList());
      });
    }

    void buildHeader(FormGroup *window)
    {
      LcdFlags flags = 0;
      if (LCD_W < LCD_H) {
        flags = FONT(XS);
      }

      // page title
      header.setTitle(STR_EDIT_THEME);
      _themeName = new StaticText(window,
                     {PAGE_TITLE_LEFT, PAGE_TITLE_TOP + PAGE_LINE_HEIGHT,
                      LCD_W - PAGE_TITLE_LEFT, PAGE_LINE_HEIGHT},
                     _theme.getName(), 0, COLOR_THEME_PRIMARY2 | flags);

      // save and cancel
      rect_t r = {LCD_W - (BUTTON_WIDTH + 5), 6, BUTTON_WIDTH, BUTTON_HEIGHT };
      _detailButton = new TextButton(window, r, STR_DETAILS, [=] () {
        new ThemeDetailsDialog(page, _theme, [=] (ThemeFile t) {
          _theme.setAuthor(t.getAuthor());
          _theme.setInfo(t.getInfo());
          _theme.setName(t.getName());

          // update the theme name
          _themeName->setText(_theme.getName());
          _dirty = true;
        });
        return 0;
      });
    }

    void buildBody(FormGroup *window)
    {
      rect_t r = { LEFT_LIST_OFFSET, TOP_LIST_OFFSET, COLOR_LIST_WIDTH, COLOR_LIST_HEIGHT};
      _cList = new ColorList(window, r, _theme.getColorList());
      _cList->setLongPressHandler([=] () { editColorPage(); });
      _cList->setPressHandler([=] () { editColorPage(); });

      if (LCD_W > LCD_H) {
        r = { 
          COLOR_LIST_WIDTH + MARGIN_WIDTH + LEFT_LIST_OFFSET, 
          TOP_LIST_OFFSET, 
          LCD_W - COLOR_LIST_WIDTH - LEFT_LIST_OFFSET - MARGIN_WIDTH * 2, 
          COLOR_LIST_HEIGHT 
        };
      } else {
        r = { 
          LEFT_LIST_OFFSET, 
          TOP_LIST_OFFSET + COLOR_LIST_HEIGHT + MARGIN_WIDTH,  
          LEFT_LIST_WIDTH, 
          COLOR_LIST_HEIGHT - (MARGIN_WIDTH * 2) 
        };
      }
      _previewWindow = new PreviewWindow(window, r, _theme.getColorList());
    }

  protected:
    bool _dirty = false;
    ThemeFile _theme;
    Page *page;
    std::function<void(ThemeFile &theme)> saveHandler = nullptr;
    PreviewWindow *_previewWindow = nullptr;
    ColorList *_cList = nullptr;
    StaticText *_themeName = nullptr;
    TextButton *_saveButton;
    TextButton *_detailButton;
};

ThemeSetupPage::ThemeSetupPage() : PageTab(STR_THEME_EDITOR, ICON_RADIO_EDIT_THEME) {}
ThemeSetupPage::~ThemeSetupPage()
{
}
void ThemeSetupPage::setAuthor(ThemeFile *theme)
{
  char author[256];
  strcpy(author, "By: ");
  strcat(author, theme->getAuthor());
  authorText->setText(author);
}


bool isTopWindow(Window *window)
{
  Window *parent = window->getParent();
  if (parent != nullptr) {
    parent = parent->getParent();
    return parent == Layer::back();
  }
  return false;
}

void ThemeSetupPage::checkEvents()
{
  PageTab::checkEvents();

  fileCarosell->pause(!isTopWindow(pageWindow));
}

void ThemeSetupPage::displayThemeMenu(Window *window, ThemePersistance *tp)
{
  auto menu = new Menu(listBox,false);

  // you can't activate the active theme
  if (listBox->getSelected() != tp->getThemeIndex()) {
    menu->addLine(STR_ACTIVATE, [=]() {
      auto idx = listBox->getSelected();
      tp->applyTheme(idx);
      tp->setDefaultTheme(idx);
      nameText->setTextFlags(COLOR_THEME_PRIMARY1);
      authorText->setTextFlags(COLOR_THEME_PRIMARY1);
      listBox->setActiveItem(idx);
    });
  }

  // you can't edit the default theme
  if (listBox->getSelected() != 0) {
    menu->addLine(STR_EDIT, [=]() {
      auto themeIdx = listBox->getSelected();
      if (themeIdx < 0) return;

      auto theme = tp->getThemeByIndex(themeIdx);
      if (theme == nullptr) return;

      new ThemeEditPage(*theme, [=](ThemeFile &theme) {
        theme.serialize();
        
        // if the theme info currently displayed
        // were changed, update the UI
        if (themeIdx == currentTheme) {
          setAuthor(&theme);
          nameText->setText(theme.getName());
          listBox->setName(currentTheme, theme.getName());
          themeColorPreview->setColorList(theme.getColorList());
        }

        // if the active theme changed, re-apply it
        if (themeIdx == tp->getThemeIndex()) theme.applyTheme();

        // update cached theme data
        auto tp_theme = tp->getThemeByIndex(themeIdx);
        if (!tp_theme) return;
        *tp_theme = theme;
      });
    });
  }

  menu->addLine(STR_DUPLICATE, [=] () {
    ThemeFile newTheme;

    new ThemeDetailsDialog(window, newTheme, [=](ThemeFile theme) {
      if (strlen(theme.getName()) != 0) {
        char name[NAME_LENGTH + 20];
        strncpy(name, theme.getName(), NAME_LENGTH + 19);
        removeAllWhiteSpace(name);

        // use the selected themes color list to make the new theme
        auto themeIdx = listBox->getSelected();
        if (themeIdx < 0) return;

        auto selTheme = tp->getThemeByIndex(themeIdx);
        if (selTheme == nullptr) return;

        for (auto color : selTheme->getColorList())
          theme.setColor(color.colorNumber, color.colorValue);

        tp->createNewTheme(name, theme);
        tp->refresh();
        listBox->setNames(tp->getNames());
        listBox->setSelected(currentTheme);
      }
    });
  });

  // you can't delete the default theme or the currently active theme
  if (listBox->getSelected() != 0 && listBox->getSelected() != tp->getThemeIndex()) {
    menu->addLine(STR_DELETE, [=] () {
      new ConfirmDialog(
          window, STR_DELETE_THEME,
          tp->getThemeByIndex(listBox->getSelected())->getName(), [=] {
            tp->deleteThemeByIndex(listBox->getSelected());
            listBox->setNames(tp->getNames());
            currentTheme = min<int>(currentTheme, tp->getNames().size() - 1);
            listBox->setSelected(currentTheme);
          });
    });
  }
}

void ThemeSetupPage::setupListbox(FormWindow *window, rect_t r, ThemePersistance *tp)
{
  listBox = new ListBox(window, r, tp->getNames());
  listBox->setAutoEdit(true);
  listBox->setSelected(currentTheme);
  listBox->setActiveItem(tp->getThemeIndex());
  listBox->setTitle(STR_THEME + std::string("s")); // TODO: fix this!
  listBox->setLongPressHandler([=] () { displayThemeMenu(window, tp); });
  listBox->setPressHandler([=] () {
      auto value = listBox->getSelected();
      if (themeColorPreview && authorText && nameText && fileCarosell) {
        ThemeFile *theme = tp->getThemeByIndex(value);
        if (!theme) return;
        themeColorPreview->setColorList(theme->getColorList());
        setAuthor(theme);
        nameText->setText(theme->getName());
        fileCarosell->setFileNames(theme->getThemeImageFileNames());
      }
      currentTheme = value;
    });
}

void ThemeSetupPage::build(FormWindow *window)
{
  window->padAll(0);
  pageWindow = window;

  auto tp = ThemePersistance::instance();
  auto theme = tp->getCurrentTheme();
  currentTheme = tp->getThemeIndex();

  themeColorPreview = nullptr;
  listBox = nullptr;
  fileCarosell = nullptr;
  nameText = nullptr;
  authorText = nullptr;
  
  // create listbox and setup menus
  rect_t r = { LEFT_LIST_OFFSET, TOP_LIST_OFFSET, LEFT_LIST_WIDTH, LEFT_LIST_HEIGHT };
  setupListbox(window, r, tp);

  rect_t colorPreviewRect;
  if (LCD_W > LCD_H) {
    r.x = LEFT_LIST_WIDTH + MARGIN_WIDTH + LEFT_LIST_OFFSET;
    r.w = LCD_W - r.x;
    colorPreviewRect = {LEFT_LIST_WIDTH + 6, 0, COLOR_PREVIEW_WIDTH, COLOR_PREVIEW_HEIGHT};
  } else {
    r.y += LEFT_LIST_HEIGHT + 4;
    r.x = 0;
    r.w = LCD_W;
    colorPreviewRect = {0, LEFT_LIST_HEIGHT + 6, COLOR_PREVIEW_WIDTH, COLOR_PREVIEW_HEIGHT};
  }

  // setup ThemeColorPreview()
  auto colorList = theme != nullptr ? theme->getColorList() : std::vector<ColorEntry>();
  themeColorPreview = new ThemeColorPreview(window, colorPreviewRect, colorList);
  
  // setup FileCarosell()
  if (LCD_W > LCD_H) {
    r.w -= COLOR_PREVIEW_WIDTH;
    r.h = 130;
    r.x += COLOR_PREVIEW_WIDTH;
  } else {
    r.y += COLOR_PREVIEW_HEIGHT;
    r.h = LEFT_LIST_HEIGHT - COLOR_PREVIEW_HEIGHT - 50;
  }
  auto fileNames = theme != nullptr ? theme->getThemeImageFileNames() : std::vector<std::string>();
  fileCarosell = new FileCarosell(window, r, fileNames);

  // author and name of theme on right side of screen
  r.x += 7;
  r.y += 135;
  r.h = 20;
  nameText = new StaticText(window, r, theme != nullptr ? theme->getName() : "", 0, 
                            COLOR_THEME_PRIMARY1);
  if (theme != nullptr && strlen(theme->getAuthor())) {
    r.y += 20;
    authorText = new StaticText(window, r, "", 0, COLOR_THEME_PRIMARY1);
    setAuthor(theme);
  }
}
