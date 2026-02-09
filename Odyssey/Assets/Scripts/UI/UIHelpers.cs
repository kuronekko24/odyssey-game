using System;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Static utility class for creating consistent voxel-style UI elements.
    /// All elements follow the GDD art direction: chunky buttons, warm amber accents,
    /// semi-transparent dark panels, and tier-colored borders.
    /// </summary>
    public static class UIHelpers
    {
        // --- GDD Color Palette ---
        public static readonly Color Interactive = new Color32(0xd4, 0xa8, 0x55, 0xFF); // #d4a855 amber
        public static readonly Color Danger      = new Color32(0xb8, 0x54, 0x50, 0xFF); // #b85450
        public static readonly Color Safe        = new Color32(0x7a, 0x9e, 0x7e, 0xFF); // #7a9e7e
        public static readonly Color Inactive    = new Color32(0x8a, 0x85, 0x78, 0xFF); // #8a8578
        public static readonly Color PanelBg     = new Color(0x1a / 255f, 0x1e / 255f, 0x3a / 255f, 0.80f); // #1a1e3a @ 80%
        public static readonly Color PanelBgSolid = new Color32(0x1a, 0x1e, 0x3a, 0xFF);
        public static readonly Color TextWhite   = Color.white;
        public static readonly Color BorderGray  = new Color32(0x55, 0x55, 0x60, 0xFF);

        // Resource colors
        public static readonly Color Iron    = new Color32(0x6b, 0x6b, 0x6b, 0xFF);
        public static readonly Color Copper  = new Color32(0xb8, 0x73, 0x33, 0xFF);
        public static readonly Color Silicon = new Color32(0x88, 0xbb, 0xdd, 0xFF);
        public static readonly Color Carbon  = new Color32(0x3a, 0x3a, 0x3a, 0xFF);
        public static readonly Color Titanium = new Color32(0xc0, 0xc0, 0xc0, 0xFF);
        public static readonly Color Crafted = new Color32(0xd4, 0xa8, 0x55, 0xFF);

        // Tier colors (T1-T8)
        public static readonly Color[] TierColors = new Color[]
        {
            new Color32(0x8a, 0x85, 0x78, 0xFF), // T1 gray
            new Color32(0xc8, 0xc0, 0xb0, 0xFF), // T2 light gray
            new Color32(0x7a, 0x9e, 0x7e, 0xFF), // T3 green
            new Color32(0x68, 0x88, 0xa8, 0xFF), // T4 blue
            new Color32(0x8a, 0x6e, 0x9e, 0xFF), // T5 purple
            new Color32(0x6e, 0x4a, 0x8e, 0xFF), // T6 deep purple
            new Color32(0xc4, 0x7a, 0x3a, 0xFF), // T7 orange
            new Color32(0xc4, 0x9a, 0x2a, 0xFF), // T8 gold
        };

        /// <summary>
        /// Creates a panel with an Image background.
        /// </summary>
        public static RectTransform CreatePanel(Transform parent, Color bgColor, Vector2 size)
        {
            var go = new GameObject("Panel", typeof(RectTransform), typeof(Image));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.sizeDelta = size;

            var img = go.GetComponent<Image>();
            img.color = bgColor;
            img.raycastTarget = true;

            return rt;
        }

        /// <summary>
        /// Creates a panel with a voxel-style border (3D depth simulation via nested images).
        /// Outer edge is a lighter top-left highlight, darker bottom-right shadow.
        /// </summary>
        public static RectTransform CreateBorderedPanel(Transform parent, Color bgColor, Vector2 size, Color borderColor, float borderWidth = 2f)
        {
            // Outer border
            var outer = CreatePanel(parent, borderColor, size);
            outer.gameObject.name = "BorderedPanel";

            // Highlight edge (top/left lighter)
            var highlight = outer.gameObject.AddComponent<Outline>();
            highlight.effectColor = new Color(1f, 1f, 1f, 0.08f);
            highlight.effectDistance = new Vector2(1f, -1f);

            // Inner panel
            var inner = CreatePanel(outer, bgColor, Vector2.zero);
            inner.gameObject.name = "InnerPanel";
            inner.anchorMin = Vector2.zero;
            inner.anchorMax = Vector2.one;
            inner.offsetMin = new Vector2(borderWidth, borderWidth);
            inner.offsetMax = new Vector2(-borderWidth, -borderWidth);

            return outer;
        }

        /// <summary>
        /// Creates a Text element with consistent styling (white, shadow).
        /// </summary>
        public static Text CreateText(Transform parent, string text, int fontSize, Color color, TextAnchor alignment)
        {
            var go = new GameObject("Text", typeof(RectTransform), typeof(Text), typeof(Shadow));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero;
            rt.offsetMax = Vector2.zero;

            var txt = go.GetComponent<Text>();
            txt.text = text;
            txt.fontSize = fontSize;
            txt.color = color;
            txt.alignment = alignment;
            txt.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            txt.horizontalOverflow = HorizontalWrapMode.Overflow;
            txt.verticalOverflow = VerticalWrapMode.Overflow;
            txt.raycastTarget = false;

            var shadow = go.GetComponent<Shadow>();
            shadow.effectColor = new Color(0f, 0f, 0f, 0.5f);
            shadow.effectDistance = new Vector2(1f, -1f);

            return txt;
        }

        /// <summary>
        /// Creates a Text element with explicit size (not stretching to fill parent).
        /// </summary>
        public static Text CreateTextWithSize(Transform parent, string text, int fontSize, Color color, TextAnchor alignment, Vector2 size)
        {
            var txt = CreateText(parent, text, fontSize, color, alignment);
            var rt = txt.GetComponent<RectTransform>();
            rt.anchorMin = new Vector2(0.5f, 0.5f);
            rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.sizeDelta = size;
            return txt;
        }

        /// <summary>
        /// Creates a chunky voxel-style button with Image background and Text child.
        /// Includes darker edges for 3D depth effect and press-scale animation.
        /// </summary>
        public static Button CreateButton(Transform parent, string label, Color bgColor, Color textColor, UnityAction onClick)
        {
            // Outer wrapper (shadow/depth edge)
            var go = new GameObject("Button", typeof(RectTransform), typeof(Image), typeof(Button));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.sizeDelta = new Vector2(160f, 48f);

            var img = go.GetComponent<Image>();
            img.color = bgColor;
            img.raycastTarget = true;

            var btn = go.GetComponent<Button>();

            // Transition colors for voxel feel
            var colors = btn.colors;
            colors.normalColor = bgColor;
            colors.highlightedColor = bgColor * 1.1f;
            colors.pressedColor = bgColor * 0.7f;
            colors.selectedColor = bgColor;
            colors.disabledColor = Inactive;
            colors.fadeDuration = 0.05f;
            btn.colors = colors;

            // Bottom-right darker edge for 3D depth
            var outline = go.AddComponent<Outline>();
            outline.effectColor = DarkenColor(bgColor, 0.4f);
            outline.effectDistance = new Vector2(-2f, -2f);

            // Top-left highlight edge
            var highlight = go.AddComponent<Shadow>();
            highlight.effectColor = LightenColor(bgColor, 0.3f);
            highlight.effectDistance = new Vector2(1f, 1f);

            // Text label
            var textGo = new GameObject("Label", typeof(RectTransform), typeof(Text), typeof(Shadow));
            var textRt = textGo.GetComponent<RectTransform>();
            textRt.SetParent(rt, false);
            textRt.anchorMin = Vector2.zero;
            textRt.anchorMax = Vector2.one;
            textRt.offsetMin = new Vector2(4f, 2f);
            textRt.offsetMax = new Vector2(-4f, -2f);

            var txt = textGo.GetComponent<Text>();
            txt.text = label;
            txt.fontSize = 18;
            txt.color = textColor;
            txt.alignment = TextAnchor.MiddleCenter;
            txt.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            txt.raycastTarget = false;

            var textShadow = textGo.GetComponent<Shadow>();
            textShadow.effectColor = new Color(0f, 0f, 0f, 0.6f);
            textShadow.effectDistance = new Vector2(1f, -1f);

            // Add press animation component
            go.AddComponent<VoxelButtonAnim>();

            if (onClick != null)
                btn.onClick.AddListener(onClick);

            return btn;
        }

        /// <summary>
        /// Creates a progress bar / slider with a fill color.
        /// </summary>
        public static Slider CreateSlider(Transform parent, Color fillColor, float value)
        {
            // Background track
            var go = new GameObject("Slider", typeof(RectTransform), typeof(Slider));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.sizeDelta = new Vector2(200f, 16f);

            var slider = go.GetComponent<Slider>();
            slider.interactable = false;
            slider.transition = Selectable.Transition.None;

            // Track background
            var bgGo = new GameObject("Background", typeof(RectTransform), typeof(Image));
            var bgRt = bgGo.GetComponent<RectTransform>();
            bgRt.SetParent(rt, false);
            bgRt.anchorMin = Vector2.zero;
            bgRt.anchorMax = Vector2.one;
            bgRt.offsetMin = Vector2.zero;
            bgRt.offsetMax = Vector2.zero;
            bgGo.GetComponent<Image>().color = new Color(0.1f, 0.1f, 0.15f, 0.9f);

            // Fill area
            var fillArea = new GameObject("FillArea", typeof(RectTransform));
            var fillAreaRt = fillArea.GetComponent<RectTransform>();
            fillAreaRt.SetParent(rt, false);
            fillAreaRt.anchorMin = Vector2.zero;
            fillAreaRt.anchorMax = Vector2.one;
            fillAreaRt.offsetMin = new Vector2(2f, 2f);
            fillAreaRt.offsetMax = new Vector2(-2f, -2f);

            // Fill image
            var fillGo = new GameObject("Fill", typeof(RectTransform), typeof(Image));
            var fillRt = fillGo.GetComponent<RectTransform>();
            fillRt.SetParent(fillAreaRt, false);
            fillRt.anchorMin = Vector2.zero;
            fillRt.anchorMax = Vector2.one;
            fillRt.offsetMin = Vector2.zero;
            fillRt.offsetMax = Vector2.zero;
            fillGo.GetComponent<Image>().color = fillColor;

            slider.fillRect = fillRt;
            slider.value = value;

            return slider;
        }

        /// <summary>
        /// Creates a ScrollRect with viewport and content container.
        /// Returns the content RectTransform for adding children.
        /// </summary>
        public static (ScrollRect scroll, RectTransform content) CreateScrollView(Transform parent, Vector2 size)
        {
            // Root
            var go = new GameObject("ScrollView", typeof(RectTransform), typeof(ScrollRect), typeof(Image));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.sizeDelta = size;

            var scrollImg = go.GetComponent<Image>();
            scrollImg.color = new Color(0f, 0f, 0f, 0.01f); // nearly invisible bg for raycast
            scrollImg.raycastTarget = true;

            var scroll = go.GetComponent<ScrollRect>();
            scroll.horizontal = false;
            scroll.vertical = true;
            scroll.movementType = ScrollRect.MovementType.Elastic;
            scroll.elasticity = 0.1f;
            scroll.scrollSensitivity = 20f;

            // Viewport with mask
            var viewport = new GameObject("Viewport", typeof(RectTransform), typeof(Image), typeof(Mask));
            var vpRt = viewport.GetComponent<RectTransform>();
            vpRt.SetParent(rt, false);
            vpRt.anchorMin = Vector2.zero;
            vpRt.anchorMax = Vector2.one;
            vpRt.offsetMin = Vector2.zero;
            vpRt.offsetMax = Vector2.zero;

            var vpImg = viewport.GetComponent<Image>();
            vpImg.color = Color.white;
            vpImg.raycastTarget = true;
            viewport.GetComponent<Mask>().showMaskGraphic = false;

            // Content container
            var content = new GameObject("Content", typeof(RectTransform));
            var contentRt = content.GetComponent<RectTransform>();
            contentRt.SetParent(vpRt, false);
            contentRt.anchorMin = new Vector2(0f, 1f);
            contentRt.anchorMax = new Vector2(1f, 1f);
            contentRt.pivot = new Vector2(0.5f, 1f);
            contentRt.sizeDelta = new Vector2(0f, 0f);

            scroll.viewport = vpRt;
            scroll.content = contentRt;

            return (scroll, contentRt);
        }

        /// <summary>
        /// Creates a horizontal ScrollRect (for horizontal item rows).
        /// </summary>
        public static (ScrollRect scroll, RectTransform content) CreateHorizontalScrollView(Transform parent, Vector2 size)
        {
            var (scroll, content) = CreateScrollView(parent, size);
            scroll.horizontal = true;
            scroll.vertical = false;

            var contentRt = content;
            contentRt.anchorMin = new Vector2(0f, 0f);
            contentRt.anchorMax = new Vector2(0f, 1f);
            contentRt.pivot = new Vector2(0f, 0.5f);
            contentRt.sizeDelta = new Vector2(0f, 0f);

            return (scroll, contentRt);
        }

        /// <summary>
        /// Creates an InputField with placeholder text.
        /// </summary>
        public static InputField CreateInputField(Transform parent, string placeholder, int fontSize)
        {
            var go = new GameObject("InputField", typeof(RectTransform), typeof(Image), typeof(InputField));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.sizeDelta = new Vector2(160f, 40f);

            var img = go.GetComponent<Image>();
            img.color = new Color(0.08f, 0.08f, 0.14f, 0.95f);

            // Border
            var outline = go.AddComponent<Outline>();
            outline.effectColor = BorderGray;
            outline.effectDistance = new Vector2(1f, -1f);

            // Placeholder text
            var phGo = new GameObject("Placeholder", typeof(RectTransform), typeof(Text));
            var phRt = phGo.GetComponent<RectTransform>();
            phRt.SetParent(rt, false);
            phRt.anchorMin = Vector2.zero;
            phRt.anchorMax = Vector2.one;
            phRt.offsetMin = new Vector2(8f, 2f);
            phRt.offsetMax = new Vector2(-8f, -2f);

            var phTxt = phGo.GetComponent<Text>();
            phTxt.text = placeholder;
            phTxt.fontSize = fontSize;
            phTxt.color = new Color(1f, 1f, 1f, 0.3f);
            phTxt.alignment = TextAnchor.MiddleLeft;
            phTxt.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            phTxt.fontStyle = FontStyle.Italic;

            // Input text
            var textGo = new GameObject("Text", typeof(RectTransform), typeof(Text));
            var textRt = textGo.GetComponent<RectTransform>();
            textRt.SetParent(rt, false);
            textRt.anchorMin = Vector2.zero;
            textRt.anchorMax = Vector2.one;
            textRt.offsetMin = new Vector2(8f, 2f);
            textRt.offsetMax = new Vector2(-8f, -2f);

            var txt = textGo.GetComponent<Text>();
            txt.fontSize = fontSize;
            txt.color = TextWhite;
            txt.alignment = TextAnchor.MiddleLeft;
            txt.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            txt.supportRichText = false;

            var input = go.GetComponent<InputField>();
            input.textComponent = txt;
            input.placeholder = phTxt;
            input.contentType = InputField.ContentType.IntegerNumber;
            input.caretColor = Interactive;
            input.selectionColor = new Color(Interactive.r, Interactive.g, Interactive.b, 0.3f);

            return input;
        }

        /// <summary>
        /// Sets up a RectTransform to fill its parent completely.
        /// </summary>
        public static void StretchFill(RectTransform rt)
        {
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero;
            rt.offsetMax = Vector2.zero;
        }

        /// <summary>
        /// Anchor a RectTransform to a specific corner/edge.
        /// </summary>
        public static void AnchorTo(RectTransform rt, TextAnchor anchor, Vector2 offset = default)
        {
            Vector2 anchorPt;
            Vector2 pivot;

            switch (anchor)
            {
                case TextAnchor.UpperLeft:
                    anchorPt = new Vector2(0f, 1f); pivot = new Vector2(0f, 1f); break;
                case TextAnchor.UpperCenter:
                    anchorPt = new Vector2(0.5f, 1f); pivot = new Vector2(0.5f, 1f); break;
                case TextAnchor.UpperRight:
                    anchorPt = new Vector2(1f, 1f); pivot = new Vector2(1f, 1f); break;
                case TextAnchor.MiddleLeft:
                    anchorPt = new Vector2(0f, 0.5f); pivot = new Vector2(0f, 0.5f); break;
                case TextAnchor.MiddleCenter:
                    anchorPt = new Vector2(0.5f, 0.5f); pivot = new Vector2(0.5f, 0.5f); break;
                case TextAnchor.MiddleRight:
                    anchorPt = new Vector2(1f, 0.5f); pivot = new Vector2(1f, 0.5f); break;
                case TextAnchor.LowerLeft:
                    anchorPt = new Vector2(0f, 0f); pivot = new Vector2(0f, 0f); break;
                case TextAnchor.LowerCenter:
                    anchorPt = new Vector2(0.5f, 0f); pivot = new Vector2(0.5f, 0f); break;
                case TextAnchor.LowerRight:
                    anchorPt = new Vector2(1f, 0f); pivot = new Vector2(1f, 0f); break;
                default:
                    anchorPt = new Vector2(0.5f, 0.5f); pivot = new Vector2(0.5f, 0.5f); break;
            }

            rt.anchorMin = anchorPt;
            rt.anchorMax = anchorPt;
            rt.pivot = pivot;
            rt.anchoredPosition = offset;
        }

        /// <summary>
        /// Creates a colored square icon (placeholder for real sprites).
        /// </summary>
        public static Image CreateIcon(Transform parent, Color color, Vector2 size)
        {
            var go = new GameObject("Icon", typeof(RectTransform), typeof(Image));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.sizeDelta = size;

            var img = go.GetComponent<Image>();
            img.color = color;
            img.raycastTarget = false;

            return img;
        }

        // --- Color Utilities ---

        public static Color DarkenColor(Color c, float amount)
        {
            return new Color(
                Mathf.Max(0f, c.r - amount),
                Mathf.Max(0f, c.g - amount),
                Mathf.Max(0f, c.b - amount),
                c.a
            );
        }

        public static Color LightenColor(Color c, float amount)
        {
            return new Color(
                Mathf.Min(1f, c.r + amount),
                Mathf.Min(1f, c.g + amount),
                Mathf.Min(1f, c.b + amount),
                c.a
            );
        }

        /// <summary>
        /// Returns the resource color for a given resource type name.
        /// </summary>
        public static Color GetResourceColor(string resourceType)
        {
            switch (resourceType?.ToLowerInvariant())
            {
                case "iron":
                case "ferrite":     return Iron;
                case "copper":
                case "neodyne":     return Copper;
                case "silicon":
                case "quantum":     return Silicon;
                case "carbon":
                case "polymer":     return Carbon;
                case "titanium":
                case "tritanium":   return Titanium;
                default:            return Crafted;
            }
        }

        /// <summary>
        /// Returns the tier color (0-indexed: tier 1 = index 0).
        /// </summary>
        public static Color GetTierColor(int tier)
        {
            int idx = Mathf.Clamp(tier - 1, 0, TierColors.Length - 1);
            return TierColors[idx];
        }
    }

    /// <summary>
    /// Simple press animation for voxel-style buttons: scale to 0.9 on press, bounce back on release.
    /// Uses coroutine-free approach (Update-based lerp) for zero external dependencies.
    /// </summary>
    public class VoxelButtonAnim : MonoBehaviour,
        UnityEngine.EventSystems.IPointerDownHandler,
        UnityEngine.EventSystems.IPointerUpHandler
    {
        private Vector3 _originalScale = Vector3.one;
        private Vector3 _targetScale = Vector3.one;
        private float _animSpeed = 12f;

        private void Start()
        {
            _originalScale = transform.localScale;
            _targetScale = _originalScale;
        }

        public void OnPointerDown(UnityEngine.EventSystems.PointerEventData eventData)
        {
            _targetScale = _originalScale * 0.9f;
        }

        public void OnPointerUp(UnityEngine.EventSystems.PointerEventData eventData)
        {
            _targetScale = _originalScale;
        }

        private void Update()
        {
            if (Vector3.Distance(transform.localScale, _targetScale) > 0.001f)
            {
                transform.localScale = Vector3.Lerp(transform.localScale, _targetScale, Time.unscaledDeltaTime * _animSpeed);
            }
        }
    }
}
