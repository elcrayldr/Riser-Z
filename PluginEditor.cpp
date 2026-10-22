#include "PluginEditor.h"

namespace
{
    const juce::Colour kBackground (0xffe9edec);
    const juce::Colour kPanel (0xfff5f7f6);
    const juce::Colour kInk (0xff2e4b60);
    const juce::Colour kMuted (0xff8297a5);
    const juce::Colour kAccent (0xff2e4b60);
    const juce::Colour kLine (0xffc7d1d5);

    struct KnobDef { const char* id; const char* name; };

    const KnobDef kKnobDefs[] =
    {
        { "size",    "TAMAÑO" },
        { "damping", "ABSORCIÓN" },
        { "length",  "DURACIÓN (s)" },
        { "deess",   "DE-ESSER" },
        { "sweep",   "FILTRO" },
        { "drive",   "SATURACIÓN" },
        { "echo",    "ECO" },
        { "dry",     "VOZ SECA" }
    };

    const char* kCaptureIdle = "CAPTURE";
    const char* kCaptureActive = "STOP CAPTURE";
}

// ---------------- Capture button ----------------

void RiserLabEditor::CaptureButton::paintButton (juce::Graphics& g,
                                                  bool highlighted,
                                                  bool pressed)
{
    auto bounds = getLocalBounds().toFloat();
    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight()) - 8.0f;
    auto circle = juce::Rectangle<float> (diameter, diameter)
                      .withCentre (bounds.getCentre());

    g.setColour (juce::Colour (0x180e2634));
    g.fillEllipse (circle.translated (0.0f, 5.0f));

    g.setColour (kLine);
    g.drawEllipse (circle.expanded (5.0f), 1.0f);

    g.setColour (active ? juce::Colour (0xffb8d7d0) : juce::Colour (0xfff8faf9));
    g.fillEllipse (circle);

    g.setColour (highlighted ? juce::Colour (0xff45667c) : kAccent);
    g.drawEllipse (circle, pressed ? 3.0f : 2.0f);

    auto inner = circle.reduced (10.0f);
    g.setColour (active ? juce::Colour (0xffd9ebe6) : juce::Colour (0xffedf1f0));
    g.fillEllipse (inner);

    g.setColour (kMuted);
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawFittedText ("RISER-Z", inner.removeFromTop (inner.getHeight() * 0.34f).toNearestInt(),
                      juce::Justification::centred, 1);

    g.setColour (kInk);
    g.setFont (juce::Font (active ? 16.0f : 20.0f, juce::Font::bold));
    g.drawFittedText (active ? "STOP" : "CAPTURE", inner.toNearestInt(),
                      juce::Justification::centred, 1);

    g.setColour (active ? juce::Colour (0xffbd5c61) : juce::Colour (0xff3c9e91));
    g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f)
                       .withCentre ({ circle.getCentreX(), circle.getBottom() - 20.0f }));
}

// ---------------- DragArea ----------------

void RiserLabEditor::DragArea::setData (std::vector<float> newPeaks, const juce::File& newFile)
{
    peaks = std::move (newPeaks);
    file = newFile;
    repaint();
}

void RiserLabEditor::DragArea::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setColour (kPanel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (kLine);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

    if (peaks.empty())
    {
        g.setColour (kInk);
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawText (TXT ("ARRASTRA EL RISER A TU DAW"),
                    getLocalBounds().removeFromTop (24), juce::Justification::centred);
        g.setColour (kMuted);
        g.setFont (11.0f);
        g.drawText (TXT ("La forma de onda aparecerá aquí al generar"),
                    getLocalBounds().withTrimmedTop (22), juce::Justification::centred);
        return;
    }

    g.setColour (juce::Colour (0xff3c9e91));
    const float mid = r.getCentreY() - 5.0f;
    const float h = r.getHeight() * 0.31f;

    for (size_t i = 0; i < peaks.size(); ++i)
    {
        const float x = r.getX() + 10.0f + ((float) i / (float) peaks.size()) * (r.getWidth() - 20.0f);
        const float a = juce::jmax (0.01f, peaks[i]) * h;
        g.drawLine (x, mid - a, x, mid + a, 1.4f);
    }

    g.setColour (kInk);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText (TXT ("ARRASTRA ESTE AUDIO A TU DAW"),
                getLocalBounds().removeFromBottom (19), juce::Justification::centred);
}

void RiserLabEditor::DragArea::mouseDrag (const juce::MouseEvent& e)
{
    if (file.existsAsFile() && e.getDistanceFromDragStart() > 6)
        juce::DragAndDropContainer::performExternalDragDropOfFiles (
            juce::StringArray (file.getFullPathName()), false, this);
}

// ---------------- Editor ----------------

RiserLabEditor::RiserLabEditor (RiserLabProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (620, 650);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.setColour (juce::ComboBox::backgroundColourId, kPanel);
    presetBox.setColour (juce::ComboBox::textColourId, kInk);
    presetBox.setColour (juce::ComboBox::outlineColourId, kLine);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    captureButton.setClickingTogglesState (false);
    captureButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    captureButton.onClick = [this] { onCaptureClicked(); };
    addAndMakeVisible (captureButton);

    generateButton.setButtonText (TXT ("GENERAR RISER"));
    generateButton.setColour (juce::TextButton::buttonColourId, kAccent);
    generateButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    generateButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    generateButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff45667c));
    generateButton.onClick = [this] { onGenerateClicked(); };
    addAndMakeVisible (generateButton);

    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, kMuted);
    statusLabel.setFont (juce::Font (12.0f));
    addAndMakeVisible (statusLabel);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 15);
        k.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff3c9e91));
        k.slider.setColour (juce::Slider::rotarySliderOutlineColourId, kLine);
        k.slider.setColour (juce::Slider::thumbColourId, kInk);
        k.slider.setColour (juce::Slider::textBoxTextColourId, kInk);
        k.slider.setColour (juce::Slider::textBoxBackgroundColourId, kBackground);
        k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (k.slider);

        k.label.setText (TXT (kKnobDefs[i].name), juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setColour (juce::Label::textColourId, kInk);
        k.label.setFont (juce::Font (10.0f, juce::Font::bold));
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<SliderAttachment> (proc.apvts, kKnobDefs[i].id, k.slider);
    }

    cutToggle.setButtonText (TXT ("Cortar al inicio de la voz"));
    cutToggle.setColour (juce::ToggleButton::textColourId, kInk);
    addAndMakeVisible (cutToggle);
    cutAttachment = std::make_unique<ButtonAttachment> (proc.apvts, "cut", cutToggle);

    addAndMakeVisible (dragArea);

    auto peaks = proc.getRiserPeaks (300);
    if (! peaks.empty())
        dragArea.setData (std::move (peaks), proc.getRiserFile());

    startTimerHz (15);
}

RiserLabEditor::~RiserLabEditor()
{
    stopTimer();
}

void RiserLabEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBackground);

    g.setColour (kInk);
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText ("RISER-Z", titleArea, juce::Justification::centredLeft);

    g.setColour (kMuted);
    g.setFont (juce::Font (10.0f));
    g.drawText (TXT ("CAPTURE  /  REVERSE FX"), subtitleArea, juce::Justification::centredLeft);

    auto divider = getLocalBounds().reduced (18).withHeight (1).translated (0, 44);
    g.setColour (kLine);
    g.fillRect (divider);
}

void RiserLabEditor::resized()
{
    auto area = getLocalBounds().reduced (18);

    auto top = area.removeFromTop (34);
    presetBox.setBounds (top.removeFromRight (154).withHeight (26).withY (top.getY() + 3));
    titleArea = top.removeFromTop (20);
    subtitleArea = juce::Rectangle<int> (titleArea.getX(), titleArea.getBottom() + 1,
                                         titleArea.getWidth(), 12);

    area.removeFromTop (10);

    auto hero = area.removeFromTop (218);
    const int buttonSize = juce::jmin (174, hero.getWidth() / 2);
    captureButton.setBounds (juce::Rectangle<int> (buttonSize, buttonSize)
                                 .withCentre ({ hero.getCentreX(), hero.getY() + 91 }));
    statusLabel.setBounds (hero.getX(), hero.getY() + 180, hero.getWidth(), 18);
    generateButton.setBounds (juce::Rectangle<int> (142, 27)
                                  .withCentre ({ hero.getCentreX(), hero.getBottom() - 7 }));

    area.removeFromTop (4);

    for (int r = 0; r < 2; ++r)
    {
        auto knobRow = area.removeFromTop (91);
        const int w = knobRow.getWidth() / 4;
        for (int c = 0; c < 4; ++c)
        {
            auto cell = knobRow.removeFromLeft (w);
            auto& k = knobs[(size_t) (r * 4 + c)];
            k.label.setBounds (cell.removeFromTop (16));
            k.slider.setBounds (cell);
        }
    }

    area.removeFromTop (2);
    cutToggle.setBounds (area.removeFromTop (25));
    area.removeFromTop (6);
    dragArea.setBounds (area);
}

void RiserLabEditor::onCaptureClicked()
{
    if (! proc.isCapturing())
    {
        lastMessage.clear();
        proc.startCapture();
        captureButton.setActive (true);
    }
    else
    {
        proc.stopCapture();
        captureButton.setActive (false);
    }
}

void RiserLabEditor::onGenerateClicked()
{
    proc.stopCapture();
    captureButton.setActive (false);

    juce::String error;
    if (proc.generateRiser (error))
    {
        dragArea.setData (proc.getRiserPeaks (300), proc.getRiserFile());
        lastMessage = TXT ("Riser listo. Arrástralo a tu pista.");
    }
    else
    {
        lastMessage = error;
    }
}

void RiserLabEditor::timerCallback()
{
    const bool capturing = proc.isCapturing();

    if (! capturing && captureButton.active)
        captureButton.setActive (false);

    juce::String msg;
    const double seconds = proc.getCapturedSeconds();

    if (capturing)
        msg = TXT ("CAPTURANDO... ") + juce::String (seconds, 1) + " s";
    else if (lastMessage.isNotEmpty())
        msg = lastMessage;
    else if (seconds > 0.0)
        msg = TXT ("Capturado: ") + juce::String (seconds, 1) + " s";
    else
        msg = TXT ("Pulsa CAPTURE y reproduce el audio en tu DAW");

    if (statusLabel.getText() != msg)
        statusLabel.setText (msg, juce::dontSendNotification);
}
