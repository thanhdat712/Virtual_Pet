const entryGate = document.querySelector('#entry-gate');
const entryActions = document.querySelector('#entry-actions');
const entryForm = document.querySelector('#entry-form');
const entryError = document.querySelector('#entry-error');
const entrySubmit = document.querySelector('#submit-auth');
const petPicker = document.querySelector('#pet-picker');
const petChoiceError = document.querySelector('#pet-choice-error');
const renameGate = document.querySelector('#rename-dialog');
const renameForm = document.querySelector('#rename-form');
const renameMessage = document.querySelector('#rename-message');
const settingsPanel = document.querySelector('#settings-panel');
const settingsMessage = document.querySelector('#settings-message');
const settingsSoundToggle = document.querySelector('#settings-sound-toggle');
let signedInUser = null;
let authMode = 'login';
let selectedSpecies = 'cat';

function setSoundEnabled(enabled) {
	window.soundEnabled = enabled;
	settingsSoundToggle.checked = enabled;
	document.querySelectorAll('audio').forEach((audio) => { audio.muted = !enabled; });
	window.dispatchEvent(new CustomEvent('soundsettingchange', { detail: { enabled } }));
}
window.applySoundPreference = setSoundEnabled;

function openSettings() {
	settingsMessage.textContent = '';
	document.querySelector('#settings-pet-name').textContent = window.currentPetProfile?.name || signedInUser?.username || 'UIA';
	settingsPanel.hidden = false;
	document.querySelector('#close-settings').focus();
}

function closeSettings() {
	settingsPanel.hidden = true;
	document.querySelector('#open-settings').focus();
}

settingsSoundToggle.checked = true;
setSoundEnabled(true);

document.querySelector('#open-settings').addEventListener('click', openSettings);
document.querySelector('#close-settings').addEventListener('click', closeSettings);
settingsPanel.addEventListener('click', (event) => {
	if (event.target === settingsPanel) closeSettings();
});
settingsSoundToggle.addEventListener('change', async () => {
	const previous = !settingsSoundToggle.checked;
	setSoundEnabled(settingsSoundToggle.checked);
	try {
		applyAccountState(await api.saveSoundEnabled(settingsSoundToggle.checked));
	} catch (failure) {
		settingsSoundToggle.checked = previous;
		setSoundEnabled(previous);
		settingsMessage.textContent = failure.message || 'Could not save this setting.';
	}
});
document.addEventListener('keydown', (event) => {
	if (event.key === 'Escape' && !settingsPanel.hidden) closeSettings();
});

document.querySelector('#settings-logout').addEventListener('click', async (event) => {
	const button = event.currentTarget;
	button.disabled = true;
	settingsMessage.textContent = '';
	try {
		await api.logout();
		window.location.reload();
	} catch (failure) {
		settingsMessage.textContent = failure.message || 'Could not log out. Please try again.';
		button.disabled = false;
	}
});

function profileKey(user) {
	return `virtual-pet-profile-v1-${encodeURIComponent(String(user.username).toLowerCase())}`;
}

async function migrateBrowserPreferences(user) {
	let changed = false;
	try {
		const key = profileKey(user);
		const saved = JSON.parse(localStorage.getItem(key) || 'null');
		if (saved && typeof saved === 'object') {
			const name = typeof saved.name === 'string' && saved.name.trim()
				? saved.name.trim() : window.currentPetProfile?.name || 'UIA';
			const species = saved.species === 'dog' ? 'dog' : 'cat';
			applyAccountState(await api.savePetProfile(name, species));
			localStorage.removeItem(key);
			changed = true;
		}
		const oldSound = localStorage.getItem('virtual-pet-sound-enabled');
		if (oldSound === 'true' || oldSound === 'false') {
			applyAccountState(await api.saveSoundEnabled(oldSound === 'true'));
			localStorage.removeItem('virtual-pet-sound-enabled');
			changed = true;
		}
	} catch {
		return;
	}
	if (changed) await window.initializeAccount();
}

function showPetPicker(user) {
	const profile = window.currentPetProfile || { species: 'cat', name: user.username || 'UIA' };
	entryGate.hidden = true;
	selectedSpecies = profile.species;
	document.querySelectorAll('[data-pet-species]').forEach((button) => {
		button.setAttribute('aria-pressed', String(button.dataset.petSpecies === selectedSpecies));
	});
	petChoiceError.textContent = '';
	petPicker.hidden = false;
}

function showAuth(mode) {
	authMode = mode;
	entryGate.hidden = false;
	entryActions.hidden = true;
	entryForm.hidden = false;
	entryError.textContent = '';
	entryError.classList.remove('is-success');
	entrySubmit.textContent = mode === 'register' ? 'Create account' : 'Log in';
	const title = document.querySelector('#entry-title');
	title.textContent = mode === 'register' ? 'Create account' : 'Log in';
	const usernameInput = document.querySelector('#entry-username');
	document.querySelector('#username-field').hidden = mode !== 'register';
	usernameInput.required = mode === 'register';
	document.querySelector('#confirm-password-field').hidden = mode !== 'register';
	const password = document.querySelector('#entry-password');
	password.autocomplete = mode === 'register' ? 'new-password' : 'current-password';
	password.placeholder = mode === 'register' ? 'At least 8 characters' : 'Enter your password';
	document.querySelector('#entry-email').focus();
}

document.querySelector('#show-login').addEventListener('click', () => showAuth('login'));
document.querySelector('#show-register').addEventListener('click', () => showAuth('register'));
document.querySelector('#entry-back').addEventListener('click', () => {
	entryForm.reset();
	entryForm.hidden = true;
	entryActions.hidden = false;
	entryError.textContent = '';
	entrySubmit.disabled = false;
	entryGate.querySelector('#entry-title').textContent = 'Welcome!';
});

document.querySelectorAll('.password-toggle, [data-password-target]').forEach((button) => {
	button.addEventListener('click', () => {
		const targetId = button.dataset.passwordTarget || 'entry-password';
		const input = document.getElementById(targetId);
		const visible = input.type === 'password';
		input.type = visible ? 'text' : 'password';
		button.classList.toggle('is-visible', visible);
		button.setAttribute('aria-label', visible ? 'Hide password' : 'Show password');
		button.setAttribute('aria-pressed', String(visible));
	});
});

document.querySelectorAll('[data-pet-species]').forEach((button) => {
	button.addEventListener('click', () => {
		selectedSpecies = button.dataset.petSpecies;
		document.querySelectorAll('[data-pet-species]').forEach((choice) => {
			choice.setAttribute('aria-pressed', String(choice === button));
		});
	});
});

document.querySelector('#confirm-pet-choice').addEventListener('click', async (event) => {
	const button = event.currentTarget;
	button.disabled = true;
	try {
		const petName = window.currentPetProfile?.name || signedInUser.username || 'UIA';
		applyAccountState(await api.savePetProfile(petName, selectedSpecies));
		applyLobbyDecor();
		petPicker.hidden = true;
		entryGate.hidden = true;
		document.body.classList.remove('entry-open');
	} catch (failure) {
		petChoiceError.textContent = failure.message || 'Could not save your pet choice.';
	} finally {
		button.disabled = false;
	}
});

document.querySelector('#settings-rename').addEventListener('click', () => {
	if (!signedInUser) return;
	document.querySelector('#pet-new-name').value = window.currentPetProfile?.name || 'UIA';
	renameMessage.textContent = '';
	renameGate.hidden = false;
	document.querySelector('#pet-new-name').focus();
	document.querySelector('#pet-new-name').select();
});
document.querySelector('#cancel-rename').addEventListener('click', () => { renameGate.hidden = true; });
renameForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	const name = document.querySelector('#pet-new-name').value.trim();
	if (!name) { renameMessage.textContent = 'Enter a name for your pet.'; return; }
	try {
		const species = window.currentPetProfile?.species || 'cat';
		applyAccountState(await api.savePetProfile(name, species));
		document.querySelector('#settings-pet-name').textContent = name;
		renameGate.hidden = true;
	} catch (failure) {
		renameMessage.textContent = failure.message || 'Could not save the pet name.';
	}
});

entryForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	entryError.textContent = '';
	entrySubmit.disabled = true;
	const username = document.querySelector('#entry-username').value.trim();
	const email = document.querySelector('#entry-email').value.trim();
	const password = document.querySelector('#entry-password').value;
	const confirmation = document.querySelector('#entry-confirm-password').value;
	if (authMode === 'register' && password !== confirmation) {
		entryError.textContent = 'The passwords do not match.';
		entrySubmit.disabled = false;
		return;
	}
	if (authMode === 'register' && Array.from(password).length < 8) {
		entryError.textContent = 'Password must be at least 8 characters long.';
		entrySubmit.disabled = false;
		return;
	}
	try {
		if (authMode === 'register') {
			await api.register(username, email, password);
			entryError.textContent = `Account created. Please log in with ${email}.`;
			entryError.classList.add('is-success');
			entryForm.reset();
			setTimeout(() => showAuth('login'), 1000);
			return;
		}
		const result = await api.login(email, password);
		signedInUser = result.user;
		entryForm.reset();
		if (!await window.initializeAccount()) throw new Error('Could not load your saved game.');
		await migrateBrowserPreferences(signedInUser);
		entryGate.hidden = true;
		showPetPicker(signedInUser);
	} catch (failure) {
		entryError.textContent = failure.message || 'Could not connect to the server. Open Virtual Pet through its server and try again.';
	} finally {
		entrySubmit.disabled = false;
	}
});

api.session()
	.then(async (result) => {
		if (result.authenticated) {
			signedInUser = result.user;
			if (!await window.initializeAccount()) throw new Error('Could not load your saved game.');
			await migrateBrowserPreferences(signedInUser);
			showPetPicker(signedInUser);
		} else {
			entryGate.hidden = false;
			document.body.classList.add('entry-open');
		}
	})
	.catch(() => {
		entryGate.hidden = false;
		document.body.classList.add('entry-open');
		entryError.textContent = 'Could not check your session. Please log in.';
	});
