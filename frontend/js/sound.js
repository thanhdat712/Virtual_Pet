(() => {
	const AudioContextClass = window.AudioContext || window.webkitAudioContext;
	let context;
	let masterGain;
	let enabled = true;
	let lastClickAt = 0;
	const volumeSlider = document.querySelector('#settings-sound-volume');
	const volumeLabel = document.querySelector('#settings-sound-volume-value');
	let volume = 70;
	try {
		const savedVolume = localStorage.getItem('pet-home-sound-volume');
		if (savedVolume !== null) {
			const parsedVolume = Number(savedVolume);
			if (Number.isFinite(parsedVolume) && parsedVolume >= 0 && parsedVolume <= 100) volume = parsedVolume;
		}
	} catch {
		// Keep the in-memory default when browser storage is unavailable.
	}
	volumeSlider.value = String(volume);
	volumeLabel.value = `${volume}%`;
	volumeLabel.textContent = `${volume}%`;

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

	function playGameSound(kind = 'click') {
		if (!enabled || !AudioContextClass) return;
		if (kind === 'click') {
			const now = performance.now();
			if (now - lastClickAt < 55) return;
			lastClickAt = now;
		}
		try {
			context ||= new AudioContextClass();
			if (!masterGain) {
				masterGain = context.createGain();
				masterGain.gain.value = volume / 100;
				masterGain.connect(context.destination);
			}
			if (context.state === 'suspended') context.resume().catch(() => {});
			if (kind === 'eat') {
				tone(310, 0, 0.09, 'triangle', 0.075, 230);
				tone(520, 0.1, 0.13, 'sine', 0.08, 680);
			} else if (kind === 'sleep') {
				tone(440, 0, 0.18, 'sine', 0.065, 392);
				tone(349, 0.17, 0.2, 'sine', 0.06, 311);
				tone(261, 0.35, 0.28, 'sine', 0.055, 220);
			} else {
				tone(560, 0, 0.045, 'triangle', 0.045, 460);
			}
		} catch {
			// Audio is optional; unsupported devices should keep all game actions working.
		}
	}

	window.playGameSound = playGameSound;
	volumeSlider.addEventListener('input', () => {
		volume = Number(volumeSlider.value);
		volumeLabel.value = `${volume}%`;
		volumeLabel.textContent = `${volume}%`;
		try {
			localStorage.setItem('pet-home-sound-volume', String(volume));
		} catch {
			// The current slider value still applies for this page session.
		}
		if (masterGain) masterGain.gain.setTargetAtTime(volume / 100, context.currentTime, 0.025);
	});
	window.addEventListener('soundsettingchange', (event) => {
		enabled = event.detail?.enabled !== false;
	});

	document.addEventListener('pointerdown', (event) => {
		if (event.target instanceof Element && event.target.closest('button,[role="button"]')) {
			playGameSound('click');
		}
	}, true);

	document.addEventListener('keydown', (event) => {
		if ((event.key === 'Enter' || event.key === ' ') && !event.repeat
			&& event.target instanceof Element && event.target.closest('button,[role="button"]')) {
			playGameSound('click');
		}
	}, true);
})();
