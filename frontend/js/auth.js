const entryGate = document.querySelector('#entry-gate');
const entryActions = document.querySelector('#entry-actions');
const entryForm = document.querySelector('#entry-form');
const entryError = document.querySelector('#entry-error');
const entrySubmit = document.querySelector('#submit-auth');
const entryTitle = document.querySelector('#entry-title');
const entryDescription = entryTitle.nextElementSibling;
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

const resetStyles = document.createElement('style');
resetStyles.textContent = `
	.forgot-password-link { justify-self: start; padding: 2px 0; border: 0; background: transparent; color: #267b61; font: inherit; text-decoration: none; box-shadow: none; transition: none; cursor: pointer; }
	.forgot-password-link:hover:not(:disabled), .forgot-password-link:active:not(:disabled) { transform: none; box-shadow: none; filter: none; }
	.password-reset-panel { text-align: left; }
	.password-reset-panel h1 { text-align: center; }
	.password-reset-panel .entry-description { text-align: center; }
	.entry-error.is-success { color: #267b61; }
`;
document.head.append(resetStyles);

const forgotPasswordButton = document.createElement('button');
forgotPasswordButton.className = 'forgot-password-link';
forgotPasswordButton.type = 'button';
forgotPasswordButton.textContent = 'Forgot password?';
entrySubmit.before(forgotPasswordButton);

const passwordResetPanel = document.createElement('section');
passwordResetPanel.className = 'entry-form password-reset-panel';
passwordResetPanel.hidden = true;
passwordResetPanel.innerHTML = `
	<h1>Reset password</h1>
	<p class="entry-description">Request a one-time reset code, then choose a new password.</p>
	<p class="entry-description">Local development: the code will appear here. Email delivery is not configured.</p>
	<p id="reset-request-message" class="entry-error" role="status" aria-live="polite"></p>
	<form id="password-reset-request-form" class="entry-form">
		<label for="reset-email">Email</label>
		<input id="reset-email" name="email" type="email" maxlength="254" autocomplete="email" required>
		<button class="entry-primary" type="submit">Request reset code</button>
		<button class="entry-back" type="button" data-reset-back>Back to login</button>
	</form>
	<form id="password-reset-confirm-form" class="entry-form" hidden>
		<label for="reset-code">One-time reset code</label>
		<input id="reset-code" name="resetCode" type="text" maxlength="64" autocomplete="off" required>
		<label for="reset-new-password">New password</label>
		<input id="reset-new-password" name="password" type="password" minlength="8" maxlength="128" autocomplete="new-password" required>
		<label for="reset-confirm-password">Confirm new password</label>
		<input id="reset-confirm-password" name="confirmPassword" type="password" minlength="8" maxlength="128" autocomplete="new-password" required>
		<p id="reset-confirm-message" class="entry-error" role="alert" aria-live="polite"></p>
		<button class="entry-primary" type="submit">Set new password</button>
		<button class="entry-back" type="button" data-reset-back>Back to login</button>
	</form>`;
entryForm.insertAdjacentElement('afterend', passwordResetPanel);

const resetRequestForm = passwordResetPanel.querySelector('#password-reset-request-form');
const resetConfirmForm = passwordResetPanel.querySelector('#password-reset-confirm-form');
const resetRequestMessage = passwordResetPanel.querySelector('#reset-request-message');
const resetConfirmMessage = passwordResetPanel.querySelector('#reset-confirm-message');

const emailVerificationPanel = document.createElement('section');
emailVerificationPanel.className = 'entry-form password-reset-panel';
emailVerificationPanel.hidden = true;
emailVerificationPanel.innerHTML = `
	<h1>Verify your email</h1>
	<p class="entry-description">Enter the one-time code to activate your account.</p>
	<p class="entry-description">Local development: the code appears here because email delivery is not configured.</p>
	<p id="email-verification-message" class="entry-error" role="status" aria-live="polite"></p>
	<form id="email-verification-form" class="entry-form">
		<label for="verification-email">Email</label>
		<input id="verification-email" name="email" type="email" maxlength="254" autocomplete="email" required>
		<label for="verification-code">Verification code</label>
		<input id="verification-code" name="verificationCode" type="text" maxlength="64" autocomplete="off" required>
		<button class="entry-primary" type="submit">Verify email</button>
		<button id="resend-verification-code" class="entry-back" type="button">Resend code</button>
		<button class="entry-back" type="button" data-verification-back>Back to login</button>
	</form>`;
passwordResetPanel.insertAdjacentElement('afterend', emailVerificationPanel);

const verificationForm = emailVerificationPanel.querySelector('#email-verification-form');
const verificationMessage = emailVerificationPanel.querySelector('#email-verification-message');
const verificationEmail = emailVerificationPanel.querySelector('#verification-email');

function showEmailVerification(email, message = '') {
	entryForm.hidden = true;
	entryTitle.hidden = true;
	entryDescription.hidden = true;
	passwordResetPanel.hidden = true;
	emailVerificationPanel.hidden = false;
	verificationEmail.value = email;
	verificationMessage.classList.remove('is-success');
	verificationMessage.textContent = message;
	emailVerificationPanel.querySelector('#verification-code').focus();
}

emailVerificationPanel.querySelectorAll('[data-verification-back]').forEach((button) => {
	button.addEventListener('click', () => showAuth('login'));
});

emailVerificationPanel.querySelector('#resend-verification-code').addEventListener('click', async (event) => {
	const button = event.currentTarget;
	button.disabled = true;
	verificationMessage.classList.remove('is-success');
	verificationMessage.textContent = '';
	try {
		const result = await api.requestEmailVerification(verificationEmail.value.trim());
		verificationMessage.classList.add('is-success');
		verificationMessage.textContent = result.verificationCode
			? `New verification code (valid for 30 minutes): ${result.verificationCode}`
			: 'If the account needs verification, a new code was generated.';
	} catch (failure) {
		verificationMessage.textContent = failure.message || 'Could not resend the verification code.';
	} finally {
		button.disabled = false;
	}
});

verificationForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	const button = verificationForm.querySelector('[type="submit"]');
	button.disabled = true;
	verificationMessage.classList.remove('is-success');
	verificationMessage.textContent = '';
	try {
		const result = await api.verifyEmail(verificationEmail.value.trim(),
			emailVerificationPanel.querySelector('#verification-code').value.trim());
		showAuth('login');
		entryError.classList.add('is-success');
		entryError.textContent = result.message || 'Email verified. You can now log in.';
	} catch (failure) {
		verificationMessage.textContent = failure.message || 'Could not verify this email.';
	} finally {
		button.disabled = false;
	}
});

forgotPasswordButton.addEventListener('click', () => {
	passwordResetPanel.querySelector('#reset-email').value = document.querySelector('#entry-email').value.trim();
	resetRequestMessage.textContent = '';
	resetConfirmMessage.textContent = '';
	resetRequestForm.hidden = false;
	resetConfirmForm.hidden = true;
	entryForm.hidden = true;
	entryTitle.hidden = true;
	entryDescription.hidden = true;
	passwordResetPanel.hidden = false;
	passwordResetPanel.querySelector('#reset-email').focus();
});

passwordResetPanel.querySelectorAll('[data-reset-back]').forEach((button) => {
	button.addEventListener('click', () => showAuth('login'));
});

resetRequestForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	const button = resetRequestForm.querySelector('[type="submit"]');
	button.disabled = true;
	resetRequestMessage.classList.remove('is-success');
	resetRequestMessage.textContent = '';
	try {
		const result = await api.requestPasswordReset(passwordResetPanel.querySelector('#reset-email').value.trim());
		resetRequestMessage.classList.add('is-success');
		resetRequestMessage.textContent = result.resetCode
			? `Your one-time reset code (valid for 30 minutes): ${result.resetCode}`
			: 'If an account uses that email, a reset code was generated. Enter the code if you received it.';
		resetRequestForm.hidden = true;
		resetConfirmForm.hidden = false;
		passwordResetPanel.querySelector('#reset-code').focus();
	} catch (failure) {
		resetRequestMessage.textContent = failure.message || 'Could not request a reset code.';
	} finally {
		button.disabled = false;
	}
});

resetConfirmForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	const password = passwordResetPanel.querySelector('#reset-new-password').value;
	const confirmation = passwordResetPanel.querySelector('#reset-confirm-password').value;
	if (password !== confirmation) {
		resetConfirmMessage.textContent = 'The passwords do not match.';
		return;
	}
	const button = resetConfirmForm.querySelector('[type="submit"]');
	button.disabled = true;
	resetConfirmMessage.textContent = '';
	resetConfirmMessage.classList.remove('is-success');
	try {
		const resetCode = passwordResetPanel.querySelector('#reset-code').value.trim();
		const result = await api.resetPassword(resetCode,password);
		resetConfirmMessage.classList.add('is-success');
		resetConfirmMessage.textContent = result.message || 'Password reset. Please log in.';
		resetConfirmForm.reset();
		setTimeout(() => showAuth('login'), 1500);
	} catch (failure) {
		resetConfirmMessage.textContent = failure.message || 'Could not reset the password.';
	} finally {
		button.disabled = false;
	}
});

function setSoundEnabled(enabled) {
	window.soundEnabled = enabled;
	settingsSoundToggle.checked = enabled;
	document.querySelectorAll('audio').forEach((audio) => { audio.muted = !enabled; });
	window.dispatchEvent(new CustomEvent('soundsettingchange', { detail: { enabled } }));
}
window.applySoundPreference = setSoundEnabled;

function openSettings() {
	settingsMessage.textContent = '';
	document.querySelector('#settings-pet-name').textContent = window.currentPetProfile?.name || 'UIA';
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

function showPetPicker() {
	const profile = window.currentPetProfile || { species: 'cat', name: 'UIA' };
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
	passwordResetPanel.hidden = true;
		emailVerificationPanel.hidden = true;
	entryTitle.hidden = false;
	entryDescription.hidden = false;
	forgotPasswordButton.hidden = mode !== 'login';
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
		const petName = window.currentPetProfile?.name || 'UIA';
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
			const result = await api.register(username, email, password);
			entryForm.reset();
			const localCodeMessage = result.verificationCode
				? `Verification code for local testing (valid 30 minutes): ${result.verificationCode}`
				: 'Check your email for the verification code.';
			showEmailVerification(email,localCodeMessage);
			return;
		}
		const result = await api.login(email, password);
		signedInUser = result.user;
		entryForm.reset();
		if (!await window.initializeAccount()) throw new Error('Could not load your saved game.');
		await migrateBrowserPreferences(signedInUser);
		entryGate.hidden = true;
		showPetPicker();
	} catch (failure) {
		if (authMode === 'login' && failure.code === 'EMAIL_NOT_VERIFIED') {
			showEmailVerification(email,failure.message);
		} else {
			entryError.textContent = failure.message || 'Could not connect to the server. Open Virtual Pet through its server and try again.';
		}
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
			showPetPicker();
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
