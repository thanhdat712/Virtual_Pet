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
	.account-profile-gate { z-index: 2200; overflow-y: auto; }
	.account-profile-card { display: grid; gap: 12px; width: min(100%, 480px); max-height: calc(100dvh - 32px); overflow-y: auto; text-align: left; }
	.account-profile-card > h1, .account-profile-card > .entry-kicker { text-align: center; }
	.account-profile-card h2 { margin: 4px 0; font-size: 18px; }
	.account-profile-details { display: grid; gap: 8px; margin: 0; padding: 10px; border: 2px solid #5ca98c; background: #f5fff8; }
	.account-profile-details > div { display: flex; justify-content: space-between; gap: 12px; }
	.account-profile-details dd { max-width: 65%; margin: 0; overflow-wrap: anywhere; text-align: right; }
	.account-profile-card form { padding-top: 10px; border-top: 1px solid #8bcaaa; }
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

const accountProfilePanel = document.createElement('section');
accountProfilePanel.className = 'entry-gate account-profile-gate';
accountProfilePanel.hidden = true;
accountProfilePanel.innerHTML = `
	<div class="entry-card account-profile-card">
		<p class="entry-kicker">PET HOME ACCOUNT</p>
		<h1>Account profile</h1>
		<p id="account-profile-message" class="entry-error" role="status" aria-live="polite"></p>
		<dl class="account-profile-details">
			<div><dt>Username</dt><dd id="account-profile-username">—</dd></div>
			<div><dt>Email</dt><dd id="account-profile-email">—</dd></div>
			<div><dt>Email status</dt><dd id="account-profile-email-status">—</dd></div>
		</dl>
		<form id="account-password-form" class="entry-form">
			<h2>Change password</h2>
			<label for="account-current-password">Current password</label>
			<input id="account-current-password" type="password" autocomplete="current-password" required>
			<label for="account-new-password">New password</label>
			<input id="account-new-password" type="password" minlength="8" maxlength="128" autocomplete="new-password" required>
			<label for="account-confirm-password">Confirm new password</label>
			<input id="account-confirm-password" type="password" minlength="8" maxlength="128" autocomplete="new-password" required>
			<button class="entry-primary" type="submit">Save new password</button>
		</form>
		<form id="account-email-request-form" class="entry-form">
			<h2>Change email</h2>
			<label for="account-email-current-password">Current password</label>
			<input id="account-email-current-password" type="password" autocomplete="current-password" required>
			<label for="account-new-email">New email address</label>
			<input id="account-new-email" type="email" maxlength="254" autocomplete="email" required>
			<button class="entry-primary" type="submit">Send verification code</button>
		</form>
		<p id="account-email-verification-message" class="entry-error" role="status" aria-live="polite"></p>
		<form id="account-email-confirm-form" class="entry-form" hidden>
			<label for="account-email-verification-code">Code for new email</label>
			<input id="account-email-verification-code" type="text" maxlength="64" autocomplete="off" required>
			<p class="entry-description">Local development: the code appears here. Email delivery is not configured.</p>
			<button class="entry-primary" type="submit">Verify and change email</button>
		</form>
		<button id="close-account-profile" class="entry-back" type="button">Back to settings</button>
	</div>`;
document.body.append(accountProfilePanel);

const accountProfileMessage = accountProfilePanel.querySelector('#account-profile-message');
const accountEmailMessage = accountProfilePanel.querySelector('#account-email-verification-message');
const accountEmailConfirmForm = accountProfilePanel.querySelector('#account-email-confirm-form');
const accountEmailRequestForm = accountProfilePanel.querySelector('#account-email-request-form');

async function refreshAccountProfile() {
	const result = await api.accountProfile();
	document.querySelector('#account-profile-username').textContent = result.user.username;
	document.querySelector('#account-profile-email').textContent = result.user.email;
	document.querySelector('#account-profile-email-status').textContent = result.user.emailVerified ? 'Verified' : 'Not verified';
}

async function openAccountProfile() {
	accountProfilePanel.hidden = false;
	accountProfileMessage.textContent = 'Loading account profile…';
	accountEmailMessage.textContent = '';
	accountEmailRequestForm.hidden = false;
	accountEmailConfirmForm.hidden = true;
	try {
		await refreshAccountProfile();
		accountProfileMessage.textContent = '';
	} catch (failure) {
		accountProfileMessage.textContent = failure.message || 'Could not load account profile.';
	}
}

function closeAccountProfile() {
	accountProfilePanel.hidden = true;
	accountProfilePanel.querySelector('#account-password-form').reset();
	accountEmailRequestForm.reset();
	accountEmailConfirmForm.reset();
	accountProfileMessage.textContent = '';
	accountEmailMessage.textContent = '';
	document.querySelector('#open-account-profile').focus();
}

accountProfilePanel.addEventListener('click', (event) => {
	if (event.target === accountProfilePanel) closeAccountProfile();
});
accountProfilePanel.querySelector('#close-account-profile').addEventListener('click', closeAccountProfile);

accountProfilePanel.querySelector('#account-password-form').addEventListener('submit', async (event) => {
	event.preventDefault();
	const currentPassword = accountProfilePanel.querySelector('#account-current-password').value;
	const newPassword = accountProfilePanel.querySelector('#account-new-password').value;
	const confirmation = accountProfilePanel.querySelector('#account-confirm-password').value;
	if (newPassword !== confirmation) {
		accountProfileMessage.textContent = 'The new passwords do not match.';
		return;
	}
	const button = event.currentTarget.querySelector('[type="submit"]');
	button.disabled = true;
	accountProfileMessage.textContent = '';
	try {
		const result = await api.changePassword(currentPassword,newPassword);
		event.currentTarget.reset();
		accountProfileMessage.classList.add('is-success');
		accountProfileMessage.textContent = result.message || 'Password changed.';
	} catch (failure) {
		accountProfileMessage.classList.remove('is-success');
		accountProfileMessage.textContent = failure.message || 'Could not change the password.';
	} finally {
		button.disabled = false;
	}
});

accountEmailRequestForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	const button = event.currentTarget.querySelector('[type="submit"]');
	button.disabled = true;
	accountEmailMessage.classList.remove('is-success');
	accountEmailMessage.textContent = '';
	try {
		const result = await api.requestEmailChange(
			accountProfilePanel.querySelector('#account-email-current-password').value,
			accountProfilePanel.querySelector('#account-new-email').value.trim());
		accountEmailMessage.classList.add('is-success');
		accountEmailMessage.textContent = result.verificationCode
			? `Verification code for the new email (valid 30 minutes): ${result.verificationCode}`
			: result.message;
		accountEmailRequestForm.hidden = true;
		accountEmailConfirmForm.hidden = false;
		accountProfilePanel.querySelector('#account-email-verification-code').focus();
	} catch (failure) {
		accountEmailMessage.textContent = failure.message || 'Could not request an email change.';
	} finally {
		button.disabled = false;
	}
});

accountEmailConfirmForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	const button = event.currentTarget.querySelector('[type="submit"]');
	button.disabled = true;
	accountEmailMessage.classList.remove('is-success');
	accountEmailMessage.textContent = '';
	try {
		const result = await api.confirmEmailChange(
			accountProfilePanel.querySelector('#account-new-email').value.trim(),
			accountProfilePanel.querySelector('#account-email-verification-code').value.trim());
		await refreshAccountProfile();
		accountEmailConfirmForm.reset();
		accountEmailConfirmForm.hidden = true;
		accountEmailRequestForm.hidden = false;
		accountEmailMessage.classList.add('is-success');
		accountEmailMessage.textContent = result.message || 'Email address updated.';
	} catch (failure) {
		accountEmailMessage.textContent = failure.message || 'Could not verify the new email.';
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
	try { localStorage.setItem('pet-home-sound-enabled', String(enabled)); } catch { /* Sound still works for this page. */ }
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

const accountProfileSection = document.createElement('section');
accountProfileSection.className = 'settings-section';
accountProfileSection.innerHTML = `
	<h2>Account</h2>
	<div class="settings-row">
		<div><strong>Account profile</strong><span>Manage your email and password</span></div>
		<button id="open-account-profile" class="settings-action" type="button">Open</button>
	</div>`;
settingsPanel.querySelector('.settings-body').insertBefore(accountProfileSection,settingsMessage);
document.querySelector('#open-account-profile').addEventListener('click', openAccountProfile);

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
	if (event.key === 'Escape' && !accountProfilePanel.hidden) {
		closeAccountProfile();
		return;
	}
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
	const choiceKey = `virtual-pet-pet-choice-v1-${String(signedInUser?.id ?? signedInUser?.username ?? 'guest')}`;
	let alreadyChosen = false;
	try {
		alreadyChosen = localStorage.getItem(choiceKey) === profile.species;
		if (sessionStorage.getItem('virtual-pet-returning-from-study') === 'true') {
			sessionStorage.removeItem('virtual-pet-returning-from-study');
			localStorage.setItem(choiceKey, profile.species);
			alreadyChosen = true;
		}
	} catch {
		// The pet picker remains available when browser storage is disabled.
	}
	if (alreadyChosen) {
		petPicker.hidden = true;
		entryGate.hidden = true;
		document.body.classList.remove('entry-open');
		return;
	}
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
		try {
			const choiceKey = `virtual-pet-pet-choice-v1-${String(signedInUser?.id ?? signedInUser?.username ?? 'guest')}`;
			localStorage.setItem(choiceKey, selectedSpecies);
		} catch {
			// The saved species on the server is still available if browser storage is disabled.
		}
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
