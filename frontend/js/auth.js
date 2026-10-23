const loginDialog = document.querySelector('#login-dialog');
const registerDialog = document.querySelector('#register-dialog');
const loginForm = document.querySelector('#login-form');
const registerForm = document.querySelector('#register-form');
const loginMessage = document.querySelector('#login-message');
const registerMessage = document.querySelector('#register-message');
const loginSubmit = document.querySelector('#login-submit');
const registerSubmit = document.querySelector('#register-submit');
const accountAction = document.querySelector('#account-action');
const accountUsername = document.querySelector('#account-username');
const petPicker = document.querySelector('#pet-picker');
const petChoiceError = document.querySelector('#pet-choice-error');
const renameDialog = document.querySelector('#rename-dialog');
const renameForm = document.querySelector('#rename-form');
const renameMessage = document.querySelector('#rename-message');

let signedInUser = null;
let selectedSpecies = 'cat';

function profileKey(user) {
	return `virtual-pet-profile-v1-${encodeURIComponent(String(user.username).toLowerCase())}`;
}

function readPetProfile(user) {
	try {
		const saved = JSON.parse(localStorage.getItem(profileKey(user)) || '{}');
		return {
			species: saved.species === 'dog' ? 'dog' : 'cat',
			name: typeof saved.name === 'string' && saved.name.trim() ? saved.name.trim() : 'UIA'
		};
	} catch {
		return { species: 'cat', name: 'UIA' };
	}
}

function savePetProfile(user, profile) {
	localStorage.setItem(profileKey(user), JSON.stringify(profile));
	window.currentPetProfile = profile;
}

function applyPetProfile(profile) {
	window.currentPetProfile = profile;
	const name = profile.name || 'UIA';
	const image = profile.species === 'dog' ? 'assets/pets/gaugau.png' : 'assets/pets/meomeo.png';
	document.querySelector('#pet-name').textContent = name;
	document.querySelector('.pet-stage .pet-name').textContent = name;
	document.querySelector('.pet-stage .pet-image').src = image;
	document.querySelector('.pet-stage .pet-image').alt = `${profile.species === 'dog' ? 'Dog' : 'Cat'}, ${name}`;
	document.querySelector('.hud-avatar img').src = image;
}

function showPetPicker(user) {
	const profile = readPetProfile(user);
	window.currentPetProfile = profile;
	selectedSpecies = profile.species;
	document.querySelectorAll('[data-pet-species]').forEach((button) => {
		button.setAttribute('aria-pressed', String(button.dataset.petSpecies === selectedSpecies));
	});
	petChoiceError.textContent = '';
	petPicker.showModal();
}

function showMessage(element, text, success = false) {
	element.classList.toggle('is-success', success);
	element.textContent = text;
}

function openDialog(dialog, input, message) {
	showMessage(message, '');
	if (loginDialog.open) loginDialog.close();
	if (registerDialog.open) registerDialog.close();
	dialog.showModal();
	input.focus();
}

function setSignedInUser(user) {
	signedInUser = user;
	accountUsername.textContent = user.username;
	accountAction.textContent = 'Log out';
	accountAction.setAttribute('aria-label', `Log out of account ${user.username}`);
}

function setSignedOut() {
	signedInUser = null;
	accountUsername.textContent = 'Guest';
	accountAction.textContent = 'Log in';
	accountAction.setAttribute('aria-label', 'Log in');
}

document.querySelector('#close-login').addEventListener('click', () => {
	if (signedInUser) loginDialog.close();
});
document.querySelector('#close-register').addEventListener('click', () => {
	if (signedInUser) registerDialog.close();
});
[loginDialog, registerDialog].forEach((dialog) => dialog.addEventListener('cancel', (event) => {
	if (!signedInUser) event.preventDefault();
}));

document.querySelectorAll('[data-password-target]').forEach((button) => {
	button.addEventListener('click', () => {
		const input = document.getElementById(button.dataset.passwordTarget);
		const visible = input.type === 'password';
		input.type = visible ? 'text' : 'password';
		button.textContent = visible ? 'Hide' : 'Show';
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
		savePetProfile(signedInUser, profile);
		applyPetProfile(profile);
		petPicker.close();
		document.body.classList.remove('auth-pending');
	} catch {
		petChoiceError.textContent = 'Could not save your pet choice in this browser.';
	}
});

document.querySelector('#rename-pet').addEventListener('click', () => {
	if (!signedInUser) return;
	document.querySelector('#pet-new-name').value = window.currentPetProfile?.name || 'UIA';
	renameMessage.textContent = '';
	renameDialog.showModal();
	const input = document.querySelector('#pet-new-name');
	input.focus();
	input.select();
});
document.querySelector('#cancel-rename').addEventListener('click', () => renameDialog.close());
renameForm.addEventListener('submit', (event) => {
	event.preventDefault();
	const name = document.querySelector('#pet-new-name').value.trim();
	if (!name) { renameMessage.textContent = 'Enter a name for your pet.'; return; }
	try {
		const profile = { ...readPetProfile(signedInUser), name };
		savePetProfile(signedInUser, profile);
		applyPetProfile(profile);
		renameDialog.close();
	} catch {
		renameMessage.textContent = 'Could not save the pet name in this browser.';
	}
});

document.querySelector('#switch-to-register').addEventListener('click', () => {
	openDialog(registerDialog, document.querySelector('#register-username'), registerMessage);
});

document.querySelector('#switch-to-login').addEventListener('click', () => {
	openDialog(loginDialog, document.querySelector('#login-username'), loginMessage);
});

accountAction.addEventListener('click', async () => {
	if (!signedInUser) {
		openDialog(loginDialog, document.querySelector('#login-username'), loginMessage);
		return;
	}

	accountAction.disabled = true;
	try {
		const response = await fetch('/api/auth/logout', { method: 'POST' });
		const result = await response.json();
		if (!response.ok) {
			openDialog(loginDialog, document.querySelector('#login-username'), loginMessage);
			showMessage(loginMessage, result.message || 'Unable to log out right now. Please try again.');
			return;
		}
		setSignedOut();
		document.body.classList.add('auth-pending');
		window.currentPetProfile = null;
		openDialog(loginDialog, document.querySelector('#login-username'), loginMessage);
	} catch (error) {
		console.error('Could not contact the server to log out:', error);
		openDialog(loginDialog, document.querySelector('#login-username'), loginMessage);
		showMessage(loginMessage, 'Could not connect to the server to log out. Please try again.');
	} finally {
		accountAction.disabled = false;
	}
});

loginForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	showMessage(loginMessage, '');
	loginSubmit.disabled = true;
	loginSubmit.querySelector('span').textContent = 'Signing in...';

	const formData = new FormData(loginForm);
	const username = String(formData.get('username') ?? '').trim();
	const password = String(formData.get('password') ?? '');

	try {
		const response = await fetch('/api/auth/login', {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ username, password })
		});
		const result = await response.json();

		if (!response.ok) {
			showMessage(loginMessage, result.message || 'Unable to sign in. Please try again.');
			return;
		}

		setSignedInUser(result.user);
		loginForm.reset();
		loginDialog.close();
		showPetPicker(result.user);
	} catch {
		showMessage(loginMessage, 'Could not connect to the server. Open Virtual Pet through its server and try again.');
	} finally {
		loginSubmit.disabled = false;
		loginSubmit.querySelector('span').textContent = 'Log in';
	}
});

registerForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	showMessage(registerMessage, '');

	const formData = new FormData(registerForm);
	const username = String(formData.get('username') ?? '').trim();
	const password = String(formData.get('password') ?? '');
	const confirmation = String(formData.get('confirmPassword') ?? '');

	if (password !== confirmation) {
		showMessage(registerMessage, 'The passwords do not match.');
		document.querySelector('#register-confirm-password').focus();
		return;
	}

	if (Array.from(password).length < 8) {
		showMessage(registerMessage, 'Password must be at least 8 characters long.');
		document.querySelector('#register-password').focus();
		return;
	}

	registerSubmit.disabled = true;
	registerSubmit.querySelector('span').textContent = 'Creating account...';
	showMessage(registerMessage, 'Preparing your new home...');

	try {
		const response = await fetch('/api/auth/register', {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ username, password })
		});
		const result = await response.json();

		if (!response.ok) {
			showMessage(registerMessage, result.message || 'Unable to create your account. Please try again.');
			return;
		}

		showMessage(registerMessage, `Welcome, ${result.user.username}! Sign in to get started.`, true);
		registerForm.reset();
	} catch {
		showMessage(registerMessage, 'Could not connect to the server. Open Virtual Pet through its server and try again.');
	} finally {
		registerSubmit.disabled = false;
		registerSubmit.querySelector('span').textContent = 'Create account';
	}
});

fetch('/api/auth/session')
	.then(async (response) => {
		if (!response.ok) throw new Error(`Session check returned HTTP ${response.status}`);
		return response.json();
	})
	.then((result) => {
		if (result.authenticated) {
			setSignedInUser(result.user);
			showPetPicker(result.user);
		} else {
			setSignedOut();
			openDialog(loginDialog, document.querySelector('#login-username'), loginMessage);
		}
	})
	.catch((error) => {
		console.error('Could not restore the login session:', error);
		setSignedOut();
		openDialog(loginDialog, document.querySelector('#login-username'), loginMessage);
		showMessage(loginMessage, 'Could not check your session. Please log in.');
	});

petPicker.addEventListener('cancel', (event) => event.preventDefault());
