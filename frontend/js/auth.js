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

let signedInUser = null;

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

document.querySelector('#close-login').addEventListener('click', () => loginDialog.close());
document.querySelector('#close-register').addEventListener('click', () => registerDialog.close());

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
		if (result.authenticated) setSignedInUser(result.user);
	})
	.catch((error) => console.error('Could not restore the login session:', error));
