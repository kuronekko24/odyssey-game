using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    public enum NotificationType
    {
        Info,
        Success,
        Warning,
        Error
    }

    /// <summary>
    /// Toast-style notification system for game events.
    /// Notifications appear at top-center, slide down, stay for 3 seconds, then fade out.
    /// Stacks up to 3 notifications; newer ones push older ones up.
    /// </summary>
    public class NotificationSystem : MonoBehaviour
    {
        public static NotificationSystem Instance { get; private set; }

        private RectTransform _container;
        private readonly List<NotificationEntry> _activeNotifications = new();

        private const int MaxNotifications = 3;
        private const float DisplayDuration = 3f;
        private const float SlideInDuration = 0.25f;
        private const float FadeOutDuration = 0.4f;
        private const float NotificationHeight = 52f;
        private const float NotificationSpacing = 6f;
        private const float NotificationWidth = 500f;

        private struct NotificationEntry
        {
            public RectTransform Root;
            public CanvasGroup Group;
            public float SpawnTime;
            public float State; // 0=sliding in, 1=holding, 2=fading out, 3=done
        }

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
        }

        /// <summary>
        /// Initialize with a parent canvas transform.
        /// </summary>
        public void Initialize(Transform canvasRoot)
        {
            // Container anchored at top-center
            var go = new GameObject("NotificationContainer", typeof(RectTransform));
            _container = go.GetComponent<RectTransform>();
            _container.SetParent(canvasRoot, false);
            _container.anchorMin = new Vector2(0.5f, 1f);
            _container.anchorMax = new Vector2(0.5f, 1f);
            _container.pivot = new Vector2(0.5f, 1f);
            _container.anchoredPosition = new Vector2(0f, -20f);
            _container.sizeDelta = new Vector2(NotificationWidth, 200f);
        }

        /// <summary>
        /// Show a notification toast.
        /// </summary>
        public void ShowNotification(string message, NotificationType type)
        {
            if (_container == null) return;

            // If at max, immediately remove the oldest
            if (_activeNotifications.Count >= MaxNotifications)
            {
                RemoveNotification(0);
            }

            var entry = CreateNotificationUI(message, type);
            _activeNotifications.Add(entry);
            RepositionNotifications();
        }

        private NotificationEntry CreateNotificationUI(string message, NotificationType type)
        {
            Color borderColor = GetTypeColor(type);

            // Root container
            var go = new GameObject("Notification", typeof(RectTransform), typeof(CanvasGroup));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(_container, false);
            rt.sizeDelta = new Vector2(NotificationWidth, NotificationHeight);
            rt.anchorMin = new Vector2(0.5f, 1f);
            rt.anchorMax = new Vector2(0.5f, 1f);
            rt.pivot = new Vector2(0.5f, 1f);

            var group = go.GetComponent<CanvasGroup>();
            group.alpha = 0f; // start invisible for slide-in

            // Background panel
            var bg = UIHelpers.CreatePanel(rt, UIHelpers.PanelBg, Vector2.zero);
            UIHelpers.StretchFill(bg);

            // Colored left border
            var border = UIHelpers.CreatePanel(rt, borderColor, Vector2.zero);
            border.anchorMin = new Vector2(0f, 0f);
            border.anchorMax = new Vector2(0f, 1f);
            border.pivot = new Vector2(0f, 0.5f);
            border.sizeDelta = new Vector2(4f, 0f);
            border.anchoredPosition = Vector2.zero;

            // Message text
            var txt = UIHelpers.CreateText(rt, message, 16, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var textRt = txt.GetComponent<RectTransform>();
            textRt.anchorMin = Vector2.zero;
            textRt.anchorMax = Vector2.one;
            textRt.offsetMin = new Vector2(14f, 4f);
            textRt.offsetMax = new Vector2(-10f, -4f);
            txt.horizontalOverflow = HorizontalWrapMode.Wrap;
            txt.verticalOverflow = VerticalWrapMode.Truncate;

            return new NotificationEntry
            {
                Root = rt,
                Group = group,
                SpawnTime = Time.unscaledTime,
                State = 0f
            };
        }

        private void Update()
        {
            for (int i = _activeNotifications.Count - 1; i >= 0; i--)
            {
                var entry = _activeNotifications[i];
                float elapsed = Time.unscaledTime - entry.SpawnTime;

                if (elapsed < SlideInDuration)
                {
                    // Slide in: fade alpha from 0 to 1
                    float t = elapsed / SlideInDuration;
                    entry.Group.alpha = Mathf.Lerp(0f, 1f, EaseOutCubic(t));
                }
                else if (elapsed < SlideInDuration + DisplayDuration)
                {
                    // Holding
                    entry.Group.alpha = 1f;
                }
                else if (elapsed < SlideInDuration + DisplayDuration + FadeOutDuration)
                {
                    // Fading out
                    float t = (elapsed - SlideInDuration - DisplayDuration) / FadeOutDuration;
                    entry.Group.alpha = Mathf.Lerp(1f, 0f, EaseInCubic(t));
                }
                else
                {
                    // Done, remove
                    RemoveNotification(i);
                    continue;
                }

                _activeNotifications[i] = entry;
            }
        }

        private void RepositionNotifications()
        {
            for (int i = 0; i < _activeNotifications.Count; i++)
            {
                float targetY = -i * (NotificationHeight + NotificationSpacing);
                var rt = _activeNotifications[i].Root;
                rt.anchoredPosition = new Vector2(0f, targetY);
            }
        }

        private void RemoveNotification(int index)
        {
            if (index < 0 || index >= _activeNotifications.Count) return;

            var entry = _activeNotifications[index];
            if (entry.Root != null)
                Destroy(entry.Root.gameObject);

            _activeNotifications.RemoveAt(index);
            RepositionNotifications();
        }

        private Color GetTypeColor(NotificationType type)
        {
            switch (type)
            {
                case NotificationType.Success: return UIHelpers.Safe;
                case NotificationType.Warning: return UIHelpers.Interactive;
                case NotificationType.Error:   return UIHelpers.Danger;
                case NotificationType.Info:
                default:                       return Color.white;
            }
        }

        private static float EaseOutCubic(float t)
        {
            return 1f - Mathf.Pow(1f - t, 3f);
        }

        private static float EaseInCubic(float t)
        {
            return t * t * t;
        }
    }
}
