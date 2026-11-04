(() => {
	const AudioContextClass = window.AudioContext || window.webkitAudioContext;
	let context;
	let masterGain;
	let enabled = true;
	let lastSoundAt = 0;
	let studyMusicRequested = false;
	let gameMusicRequested = false;
	const studyMusic = document.querySelector('#study-background-music');
	const gameMusic = document.querySelector('#game-background-music');
	const soundEffects = {
		'buy-now': document.querySelector('#buy-now-sound'),
		'level-up': document.querySelector('#level-up-sound'),
		eat: document.querySelector('#eating-sound')
	};
	const studyMusicButton = document.querySelector('#study-music-toggle');
	const arcadeMusic = document.body.dataset.musicStyle === 'arcade';
	let arcadeMusicTimer = null;
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
	if (gameMusic) gameMusic.volume = Math.min(1, Math.max(0, volume / 100));

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
		if (arcadeMusicTimer !== null) {
			window.clearInterval(arcadeMusicTimer);
			arcadeMusicTimer = null;
		}
		updateStudyMusicButton();
	}
	function startGameMusic() {
		gameMusicRequested = true;
		if (!enabled || !gameMusic || document.hidden) return;
		gameMusic.volume = Math.min(1, Math.max(0, volume / 100));
		if (gameMusic.paused) gameMusic.play().catch(() => {});
	}
	function haltGameMusic() {
		gameMusic?.pause();
	}
	function updateStudyMusicButton(label) {
		if (!studyMusicButton) return;
		const playing = arcadeMusic ? arcadeMusicTimer !== null : Boolean(studyMusic && !studyMusic.paused);
		studyMusicButton.textContent = label || (playing ? '♫ Pause music' : '♫ Play music');
		studyMusicButton.setAttribute('aria-pressed', String(playing));
	}

	function startArcadeMusic() {
		if (!enabled || arcadeMusicTimer !== null || !prepareAudio()) return;
		const melody = [72, 76, 79, 76, 81, 79, 76, 72, 65, 69, 72, 69, 67, 71, 74, 71,
			72, 76, 79, 84, 83, 79, 76, 79, 69, 72, 76, 81, 79, 76, 74, 71];
		const bass = [48, 48, 53, 53, 55, 55, 50, 50];
		let step = 0;
		const playStep = () => {
			if (!enabled || !studyMusicRequested || document.hidden) return;
			const frequency = midi => 440 * Math.pow(2, (midi - 69) / 12);
			tone(frequency(melody[step]), 0, 0.115, 'square', 0.035);
			if (step % 4 === 0) tone(frequency(bass[Math.floor(step / 4) % bass.length]), 0, 0.32, 'triangle', 0.045);
			step = (step + 1) % melody.length;
		};
		const begin = () => {
			if (!enabled || !studyMusicRequested || document.hidden || arcadeMusicTimer !== null) return;
			playStep();
			arcadeMusicTimer = window.setInterval(playStep, 150);
			updateStudyMusicButton();
		};
		if (context.state === 'suspended') context.resume().then(begin).catch(() => {});
		else begin();
	}

	function startStudyMusic() {
		studyMusicRequested = true;
		if (arcadeMusic) {
			startArcadeMusic();
			return;
		}
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

	function playGameSound(soundId = 'click') {
		const soundEffect = soundEffects[soundId];
		if (soundEffect) {
			if (!enabled) return;
			soundEffect.currentTime = 0;
			soundEffect.volume = Math.min(1, Math.max(0, volume / 100));
			soundEffect.play().catch(() => {});
			return;
		}
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
		if (arcadeMusic ? arcadeMusicTimer === null : studyMusic?.paused) startStudyMusic();
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
		if (gameMusic) gameMusic.volume = Math.min(1, Math.max(0, volume / 100));
	});
	window.addEventListener('soundsettingchange', (event) => {
		enabled = event.detail?.enabled !== false;
		if (!enabled) {
			haltStudyMusic();
			haltGameMusic();
		} else {
			if (studyMusicRequested) startStudyMusic();
			if (gameMusicRequested) startGameMusic();
		}
	});
	document.addEventListener('visibilitychange', () => {
		if (document.hidden) {
			haltStudyMusic();
			haltGameMusic();
		} else {
			if (studyMusicRequested) startStudyMusic();
			if (gameMusicRequested) startGameMusic();
		}
	});
	window.addEventListener('pagehide', () => {
		stopStudyMusic();
		gameMusicRequested = false;
		haltGameMusic();
	});
	window.addEventListener('pageshow', () => {
		if (studyMusicRequested && !document.hidden) startStudyMusic();
	});

	document.addEventListener('click', (event) => {
		const action = event.target instanceof Element
			? event.target.closest('button:not(#sleep-pet):not(.food-choice):not(.purchase-confirm),[role="button"]:not(#sleep-pet):not(.food-choice),a[href],[onclick],input[type="checkbox"],select')
			: null;
		if (action) playGameSound('click');
	}, true);
	document.addEventListener('pointerdown', (event) => {
		if (!event.target.closest?.('#study-music-toggle')) startGameMusic();
	}, { capture: true, passive: true });
	document.addEventListener('keydown', (event) => {
		if (!event.target.closest?.('#study-music-toggle')) startGameMusic();
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
