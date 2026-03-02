using UnityEngine;
using Odyssey.UI;

namespace Odyssey.Audio
{
    /// <summary>
    /// Static helper class for triggering sounds from game events.
    /// All methods null-check AudioManager.Instance before playing.
    /// Provides a clean, one-call API for the rest of the codebase.
    /// </summary>
    public static class SFXTrigger
    {
        // =====================================================================
        // UI Sounds
        // =====================================================================

        /// <summary>Chunky mechanical button click.</summary>
        public static void PlayButtonClick()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayUISound(SFXLibrary.ButtonClick);
        }

        /// <summary>Ascending blip for menu/panel opening.</summary>
        public static void PlayPanelOpen()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayUISound(SFXLibrary.PanelOpen);
        }

        /// <summary>Descending blip for menu/panel closing.</summary>
        public static void PlayPanelClose()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayUISound(SFXLibrary.PanelClose);
        }

        /// <summary>Tab switch tick.</summary>
        public static void PlayTabSwitch()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayUISound(SFXLibrary.TabSwitch);
        }

        /// <summary>Error/rejected action buzz.</summary>
        public static void PlayError()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayUISound(SFXLibrary.ErrorReject);
        }

        /// <summary>OMEN currency received clink.</summary>
        public static void PlayOmenReceived()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayUISound(SFXLibrary.OmenReceived);
        }

        /// <summary>Level up fanfare -- most rewarding sound in the game.</summary>
        public static void PlayLevelUp()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayUISound(SFXLibrary.LevelUp);
        }

        /// <summary>Quest/mission complete stinger.</summary>
        public static void PlayQuestComplete()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayUISound(SFXLibrary.QuestComplete);
        }

        /// <summary>
        /// Play a notification sound based on the notification type.
        /// Maps NotificationType to appropriate sound intensity.
        /// </summary>
        public static void PlayNotification(NotificationType type)
        {
            if (AudioManager.Instance == null) return;

            switch (type)
            {
                case NotificationType.Info:
                    AudioManager.Instance.PlayUISound(SFXLibrary.NotificationPop);
                    break;
                case NotificationType.Success:
                    AudioManager.Instance.PlayUISound(SFXLibrary.OmenReceived);
                    break;
                case NotificationType.Warning:
                    AudioManager.Instance.PlayUISound(SFXLibrary.WarningAlarm);
                    break;
                case NotificationType.Error:
                    AudioManager.Instance.PlayUISound(SFXLibrary.ErrorReject);
                    break;
                default:
                    AudioManager.Instance.PlayUISound(SFXLibrary.NotificationPop);
                    break;
            }
        }

        // =====================================================================
        // Combat Sounds
        // =====================================================================

        /// <summary>Laser fire at a world position (3D positioned).</summary>
        public static void PlayLaserFire(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.LaserFire, pos);
        }

        /// <summary>Missile launch at a world position.</summary>
        public static void PlayMissileLaunch(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.MissileLaunch, pos);
        }

        /// <summary>Railgun fire -- the most satisfying crack in the game.</summary>
        public static void PlayRailgunFire(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.RailgunFire, pos);
        }

        /// <summary>Cannon fire -- deep punchy thud.</summary>
        public static void PlayCannonFire(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.CannonFire, pos);
        }

        /// <summary>EMP / Disruptor -- electrical crackle + bass drop.</summary>
        public static void PlayEmpDisruptor(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.EmpDisruptor, pos);
        }

        /// <summary>Generic hit impact at a world position.</summary>
        public static void PlayHitImpact(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.HitImpact, pos);
        }

        /// <summary>Shield hit -- electric sizzle at position.</summary>
        public static void PlayShieldHit(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.ShieldHit, pos);
        }

        /// <summary>Armor hit -- metallic clang at position.</summary>
        public static void PlayArmorHit(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.ArmorHit, pos);
        }

        /// <summary>Shield break -- descending whine at position.</summary>
        public static void PlayShieldBreak(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.ShieldBreak, pos);
        }

        /// <summary>Standard explosion at a world position.</summary>
        public static void PlayExplosion(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.Explosion, pos);
        }

        /// <summary>Ship destruction explosion -- longer, more dramatic.</summary>
        public static void PlayDeathExplosion(Vector3 pos)
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXAtPosition(SFXLibrary.DeathExplosion, pos);
        }

        // =====================================================================
        // Mining / Crafting Sounds
        // =====================================================================

        /// <summary>Mining extraction tick -- uses pitch variation per GDD (+-5%).</summary>
        public static void PlayMiningTick()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFXWithVariation(SFXLibrary.MiningTick, 0.95f, 1.05f);
        }

        /// <summary>Mining extraction complete -- ascending arpeggio.</summary>
        public static void PlayMiningComplete()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFX(SFXLibrary.MiningComplete);
        }

        /// <summary>Mining node depleted -- descending tone.</summary>
        public static void PlayNodeDepleted()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFX(SFXLibrary.NodeDepleted);
        }

        /// <summary>Crafting started -- mechanical whir.</summary>
        public static void PlayCraftStart()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFX(SFXLibrary.CraftStart);
        }

        /// <summary>Crafting completed -- triumphant anvil ring + sparkle.</summary>
        public static void PlayCraftComplete()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFX(SFXLibrary.CraftComplete);
        }

        // =====================================================================
        // Ship Sounds
        // =====================================================================

        /// <summary>Boost/afterburner activation.</summary>
        public static void PlayBoostActivate()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFX(SFXLibrary.BoostActivate);
        }

        /// <summary>Docking clamp engage/disengage.</summary>
        public static void PlayDockingClamp()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFX(SFXLibrary.DockingClamp);
        }

        /// <summary>Warp/jump transition swoosh.</summary>
        public static void PlayWarpJump()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFX(SFXLibrary.WarpJump);
        }

        /// <summary>Warning alarm -- hull low, zone danger.</summary>
        public static void PlayWarningAlarm()
        {
            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlaySFX(SFXLibrary.WarningAlarm);
        }
    }
}
