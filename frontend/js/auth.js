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
let signedInUser = null;
let authMode = 'login';
let selectedSpecies = 'cat';

function profileKey(user) {
	return `virtual-pet-profile-v1-${encodeURIComponent(String(user.username).toLowerCase())}`;
}

function readPetProfile(user) {
	const accountName = typeof user?.username === 'string' && user.username.trim()
		? user.username.trim()
		: 'UIA';
	try {
		const saved = JSON.parse(localStorage.getItem(profileKey(user)) || '{}');
		const savedName = typeof saved.name === 'string' ? saved.name.trim() : '';
		return {
			species: saved.species === 'dog' ? 'dog' : 'cat',
			name: savedName && savedName !== 'UIA' ? savedName : accountName
		};
	} catch {
		return { species: 'cat', name: accountName };
	}
}

function applyPetProfile(profile) {
	window.currentPetProfile = profile;
	const name = profile.name || 'UIA';
	const species = profile.species === 'dog' ? 'dog' : 'cat';
	const image = species === 'dog' ? 'assets/pets/gaugau.png' : 'assets/pets/meomeo.png';
	document.querySelector('#pet-name').textContent = name;
	document.querySelector('.pet-stage .pet-name').textContent = name;
	document.querySelector('.pet-stage').setAttribute('aria-label', name);
	document.querySelector('.pet-stage .pet-image').src = image;
	document.querySelector('.pet-stage .pet-image').alt = `${species === 'dog' ? 'Dog' : 'Cat'}, ${name}`;
	document.querySelector('.hud-avatar img').src = image;
}

function showPetPicker(user) {
	const profile = readPetProfile(user);
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

document.querySelector('#confirm-pet-choice').addEventListener('click', () => {
	try {
		const profile = { ...readPetProfile(signedInUser), species: selectedSpecies };
		localStorage.setItem(profileKey(signedInUser), JSON.stringify(profile));
		applyPetProfile(profile);
		applyLobbyDecor();
		petPicker.hidden = true;
		entryGate.hidden = true;
		document.body.classList.remove('entry-open');
	} catch {
		petChoiceError.textContent = 'Could not save your pet choice in this browser.';
	}
});

document.querySelector('#rename-pet').addEventListener('click', () => {
	if (!signedInUser) return;
	document.querySelector('#pet-new-name').value = window.currentPetProfile?.name || 'UIA';
	renameMessage.textContent = '';
	renameGate.hidden = false;
	document.querySelector('#pet-new-name').focus();
	document.querySelector('#pet-new-name').select();
});
document.querySelector('#cancel-rename').addEventListener('click', () => { renameGate.hidden = true; });
renameForm.addEventListener('submit', (event) => {
	event.preventDefault();
	const name = document.querySelector('#pet-new-name').value.trim();
	if (!name) { renameMessage.textContent = 'Enter a name for your pet.'; return; }
	try {
		const profile = { ...readPetProfile(signedInUser), name };
		localStorage.setItem(profileKey(signedInUser), JSON.stringify(profile));
		applyPetProfile(profile);
		renameGate.hidden = true;
	} catch {
		renameMessage.textContent = 'Could not save the pet name in this browser.';
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
		entryGate.hidden = true;
		showPetPicker(signedInUser);
		window.initializeAccount();
	} catch (failure) {
		entryError.textContent = failure.message || 'Could not connect to the server. Open Virtual Pet through its server and try again.';
	} finally {
		entrySubmit.disabled = false;
	}
});

api.session()
	.then((result) => {
		if (result.authenticated) {
			signedInUser = result.user;
			showPetPicker(signedInUser);
			window.initializeAccount();
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
