// Copyright...

#ifndef ANNOTATION_H_
#define ANNOTATION_H_

#include "public/fpdfview.h"

#include "rich_format.h"
#include "toolbox.h"

namespace formulate {

// Knobs are the little boxes around an Anotation that let you resize it.

typedef char Knobmask;
const char kTopLeftKnob      = 0b10000000;
const char kTopCenterKnob    = 0b01000000;
const char kTopRightKnob     = 0b00100000;
const char kMiddleLeftKnob   = 0b00010000;
const char kMiddleRightKnob  = 0b00001000;
const char kBottomLeftKnob   = 0b00000100;
const char kBottomCenterKnob = 0b00000010;
const char kBottomRightKnob  = 0b00000001;
const char kAllKnobs         = 0b11111111;
const char kNoKnobs          = 0;

// Annotion objects represent things on a page that the user can
// interact with. For example, editable text, freehand drawings, etc.
// Annotations may be created by user interaction (for new things) or
// from an annotation in a PDF.

// All coordinates for annotations are given in page coordinates,
// where the page origin is the upper left and positive values go to
// the right and down.

class AnnotationDelegate {
 public:
  // virtual void Redraw(int pageno, SkRect rect) {}
  // virtual void StartComposingText(int page,
  //                                 SkPoint pt,  // top-left corner
  //                                 float width,  // width, or 0 for bound text
  //                                 const char* html,  // body text
  //                                 int cursorpos,  // where to put cursor
  //                                 std::function<void(const char*)> set_text) {}
  // virtual void StopEditingText() {}
  virtual std::unique_ptr<txt::Paragraph> ParseText(const char* text) {
    return nullptr;
  }
};

class TextAnnotation;

class Annotation {
 public:
  explicit Annotation(AnnotationDelegate* delegate)
      : delegate_(delegate) {}
  virtual ~Annotation();

  bool dirty() const { return dirty_; }
  virtual Toolbox::Tool Type() { return Toolbox::kArrow_Tool; }
  static Annotation* Create(AnnotationDelegate* delegate, Toolbox::Tool type);

  // Called when creating an annotation w/ the mouse:
  virtual void CreateMouseDown(SkPoint pt);
  virtual void CreateMouseDrag(SkPoint pt);
  virtual bool CreateMouseUp(SkPoint pt);  // return true to keep this obj

  // To move the Annotation:
  // void MouseDown(SkPoint pt);
  // void MouseDrag(SkPoint pt);
  // void MouseUp(SkPoint pt);

  char KnobAtPoint(SkPoint pt) { return kNoKnobs; }

  virtual void Draw(SkCanvas* canvas, SkRect rect) = 0;
  char Knobs() const { return kAllKnobs; }
  void DrawKnobs(SkCanvas* canvas, SkRect rect);

  virtual void Flush(FPDF_DOCUMENT doc, FPDF_PAGE page) = 0;

  virtual bool Editable() const { return false; }
  virtual bool IsEditing() const { return false; }
  virtual void StartEditing(SkPoint pt) {}
  virtual void StopEditing() {}
  virtual TextAnnotation* AsTextAnnotation() { return nullptr; }

  // Called when an annotation hasn't been placed yet, but may want to draw
  virtual void SetHoverPoint(SkPoint pt);

  SkRect Bounds() const { return bounds_; }
  void SetWidth(float width) { bounds_.fRight = bounds_.fLeft + width; }
  void SetHeight(float height) { bounds_.fBottom = bounds_.fTop + height; }
  SkRect BoundsWithKnobs() const;
  SkPoint Origin() const { return SkPoint::Make(bounds_.fLeft, bounds_.fTop); }
  void Move(float dx, float dy);
  virtual void MoveKnob(Knobmask knob, float dx, float dy);

 protected:
  SkRect KnobBounds(char knob);

  AnnotationDelegate* delegate_{nullptr};  // weak ptr to containing doc
  bool dirty_{false};  // If true, PDF doesn't reflect current state
  // int page_{-1};
  SkRect bounds_;
  SkPoint down_pt_;  // where mouse down occurred

  DISALLOW_COPY_AND_ASSIGN(Annotation);
};

std::vector<std::unique_ptr<Annotation>> AnnotationsFromPage(
    AnnotationDelegate* delegate, FPDF_PAGE page);

class TextAnnotation : public Annotation {
 public:
  explicit TextAnnotation(AnnotationDelegate* delegate);
  // Load a TextAnnotation from saved PDF:
  TextAnnotation(AnnotationDelegate* delegate,
                 FPDF_PAGE page,
                 FPDF_PAGEOBJECT obj,
                 FPDF_PAGEOBJECTMARK mark);
  virtual ~TextAnnotation();
  Toolbox::Tool Type() override { return Toolbox::kText_Tool; }
  TextAnnotation* AsTextAnnotation() override { return this; }
  static constexpr char kSaveKey[] = "FN:RichText";
  static constexpr char kSaveKeyUTF16LE[] =
      "F\0N\0:\0R\0i\0c\0h\0T\0e\0x\0t\0\0";
  static const int SaveKeyUTF16LELen() { return 22; }

  bool CreateMouseUp(SkPoint pt) override;

  void SetHoverPoint(SkPoint pt) override;

  void Draw(SkCanvas* canvas, SkRect rect) override;

  void Flush(FPDF_DOCUMENT doc, FPDF_PAGE page) override;

  bool Editable() const override { return true; }
  bool IsEditing() const override { return editing_; }
  void StartEditing(SkPoint pt) override;
  void StopEditing() override;
  void SetEditingValue(const char* str) { editing_value_ = str; dirty_ = true; }
  bool fixed_width() const { return fixed_width_; }
  const std::string& editing_value() const { return editing_value_; }

  void LayoutText();
  void MoveKnob(Knobmask knob, float dx, float dy) override;

 private:
  bool editing_{false};
  std::string editing_value_;
  std::unique_ptr<txt::Paragraph> paragraph_;
  bool fixed_width_{false};
};

}  // namespace formulate

#endif  // ANNOTATION_H_
