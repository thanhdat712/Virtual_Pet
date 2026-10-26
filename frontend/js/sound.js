(() => {
	const AudioContextClass = window.AudioContext || window.webkitAudioContext;
	let context;
	let masterGain;
	let enabled = true;
	let lastSoundAt = 0;
	let studyMusicRequested = false;
	const studyMusic = document.querySelector('#study-background-music');
	const studyMusicButton = document.querySelector('#study-music-toggle');
	const volumeSlider = document.querySelector('#settings-sound-volume');
	const volumeLabel = document.querySelector('#settings-sound-volume-value');
	let volume = 70;
	try {
		const savedEnabled = localStorage.getItem('pet-home-sound-enabled');
		if (savedEnabled === 'true' || savedEnabled === 'false') enabled = savedEnabled === 'true';
		const savedVolume = localStorage.getItem('pet-home-sound-volume');
		if (savedVolume !== null) {
			const parsedVolume = Number(savedVolume);
			if (Number.isFinite(parsedVolume) && parsedVolume >= 0 && parsedVolume <= 100) volume = parsedVolume;
		}
	} catch {
		// Keep the in-memory default when browser storage is unavailable.
	}
	if (volumeSlider && volumeLabel) {
		volumeSlider.value = String(volume);
		volumeLabel.value = `${volume}%`;
		volumeLabel.textContent = `${volume}%`;
	}
	function updateStudyMusicVolume() {
		if (studyMusic) studyMusic.volume = Math.min(1, Math.max(0, volume / 100));
	}
	updateStudyMusicVolume();

	function tone(frequency, startOffset, duration, waveform, volume, endFrequency = frequency) {
		const start = context.currentTime + startOffset;
		const oscillator = context.createOscillator();
		const gain = context.createGain();
		oscillator.type = waveform;
		oscillator.frequency.setValueAtTime(frequency, start);
		if (endFrequency !== frequency) {
			oscillator.frequency.exponentialRampToValueAtTime(endFrequency, start + duration);
		}
		gain.gain.setValueAtTime(0.0001, start);
		gain.gain.exponentialRampToValueAtTime(volume, start + 0.012);
		gain.gain.exponentialRampToValueAtTime(0.0001, start + duration);
		oscillator.connect(gain);
		gain.connect(masterGain);
		oscillator.start(start);
		oscillator.stop(start + duration + 0.015);
	}

	function prepareAudio() {
		if (!AudioContextClass) return false;
		context ||= new AudioContextClass();
		if (!masterGain) {
			masterGain = context.createGain();
			masterGain.gain.value = volume / 100;
			masterGain.connect(context.destination);
		}
		return true;
	}

	function haltStudyMusic() {
		if (studyMusic) studyMusic.pause();
	}
	function updateStudyMusicButton(label) {
		if (!studyMusicButton) return;
		studyMusicButton.textContent = label || (studyMusic && !studyMusic.paused ? '♫ Pause music' : '♫ Play music');
		studyMusicButton.setAttribute('aria-pressed', String(Boolean(studyMusic && !studyMusic.paused)));
	}

	function startStudyMusic() {
		studyMusicRequested = true;
		if (!enabled || !studyMusic) return;
		updateStudyMusicVolume();
		studyMusic.play().catch((error) => {
			// Browsers may block the page-load attempt until the player interacts.
			if (error?.name !== 'NotAllowedError') {
				console.warn('Study music could not start:', error);
				updateStudyMusicButton('♫ Music unavailable');
			}
		});
	}

	function stopStudyMusic() {
		studyMusicRequested = false;
		haltStudyMusic();
	}

	function playGameSound() {
		if (!enabled || !AudioContextClass) return;
		const now = performance.now();
		if (now - lastSoundAt < 85) return;
		lastSoundAt = now;
		try {
			prepareAudio();
			if (context.state === 'suspended') context.resume().catch(() => {});
			tone(560, 0, 0.07, 'triangle', 0.06, 460);
		} catch {
			// Audio is optional; unsupported devices should keep all game actions working.
		}
	}

	window.playGameSound = playGameSound;
	window.startStudyMusic = startStudyMusic;
	window.stopStudyMusic = stopStudyMusic;
	studyMusicButton?.addEventListener('click', () => {
		if (studyMusic?.paused) startStudyMusic();
		else stopStudyMusic();
	});
	studyMusic?.addEventListener('play', () => updateStudyMusicButton());
	studyMusic?.addEventListener('pause', () => updateStudyMusicButton());
	studyMusic?.addEventListener('error', () => updateStudyMusicButton('♫ Music file error'));
	updateStudyMusicButton();
	volumeSlider?.addEventListener('input', () => {
		volume = Number(volumeSlider.value);
		if (volumeLabel) {
			volumeLabel.value = `${volume}%`;
			volumeLabel.textContent = `${volume}%`;
		}
		try {
			localStorage.setItem('pet-home-sound-volume', String(volume));
		} catch {
			// The current slider value still applies for this page session.
		}
		if (masterGain && context) masterGain.gain.setTargetAtTime(volume / 100, context.currentTime, 0.025);
		updateStudyMusicVolume();
	});
	window.addEventListener('soundsettingchange', (event) => {
		enabled = event.detail?.enabled !== false;
		if (!enabled) haltStudyMusic();
		else if (studyMusicRequested) startStudyMusic();
	});
	document.addEventListener('visibilitychange', () => {
		if (document.hidden) haltStudyMusic();
		else if (studyMusicRequested) startStudyMusic();
	});
	window.addEventListener('pagehide', stopStudyMusic);
	window.addEventListener('pageshow', () => {
		if (studyMusicRequested && !document.hidden) startStudyMusic();
	});

	document.addEventListener('click', (event) => {
		const action = event.target instanceof Element
			? event.target.closest('button:not(#sleep-pet):not(.food-choice),[role="button"]:not(#sleep-pet):not(.food-choice),a[href],[onclick],input[type="checkbox"],select')
			: null;
		if (action) playGameSound('click');
	}, true);
	// Retry the page-load autoplay attempt from a real user gesture. This also
	// covers Study controls rendered as canvas or plain div elements.
	document.addEventListener('pointerdown', (event) => {
		if (!event.target.closest?.('#study-music-toggle') && studyMusicRequested && studyMusic?.paused) startStudyMusic();
	}, { capture: true, passive: true });
	document.addEventListener('keydown', (event) => {
		if (!event.target.closest?.('#study-music-toggle') && studyMusicRequested && studyMusic?.paused) startStudyMusic();
	}, true);
})();
