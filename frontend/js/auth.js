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
	accountAction.textContent = 'Đăng xuất';
	accountAction.setAttribute('aria-label', `Đăng xuất tài khoản ${user.username}`);
}

function setSignedOut() {
	signedInUser = null;
	accountUsername.textContent = 'PetLovery';
	accountAction.textContent = 'Đăng nhập';
	accountAction.setAttribute('aria-label', 'Đăng nhập');
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
			showMessage(loginMessage, result.message || 'Chưa thể đăng xuất lúc này. Vui lòng thử lại.');
			return;
		}
		setSignedOut();
	} catch (error) {
		console.error('Could not contact the server to log out:', error);
		openDialog(loginDialog, document.querySelector('#login-username'), loginMessage);
		showMessage(loginMessage, 'Không kết nối được máy chủ để đăng xuất. Vui lòng thử lại.');
	} finally {
		accountAction.disabled = false;
	}
});

loginForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	showMessage(loginMessage, '');
	loginSubmit.disabled = true;
	loginSubmit.querySelector('span').textContent = 'Đang đăng nhập…';

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
			showMessage(loginMessage, result.message || 'Chưa thể đăng nhập. Vui lòng thử lại.');
			return;
		}

		setSignedInUser(result.user);
		loginForm.reset();
		loginDialog.close();
	} catch {
		showMessage(loginMessage, 'Không kết nối được máy chủ. Hãy mở game qua máy chủ Virtual Pet rồi thử lại.');
	} finally {
		loginSubmit.disabled = false;
		loginSubmit.querySelector('span').textContent = 'Đăng nhập';
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
		showMessage(registerMessage, 'Hai lần nhập mật khẩu chưa khớp.');
		document.querySelector('#register-confirm-password').focus();
		return;
	}

	if (Array.from(password).length < 8) {
		showMessage(registerMessage, 'Mật khẩu cần có ít nhất 8 ký tự.');
		document.querySelector('#register-password').focus();
		return;
	}

	registerSubmit.disabled = true;
	registerSubmit.querySelector('span').textContent = 'Đang tạo tài khoản…';
	showMessage(registerMessage, 'Đang chuẩn bị ngôi nhà nhỏ cho bạn…');

	try {
		const response = await fetch('/api/auth/register', {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ username, password })
		});
		const result = await response.json();

		if (!response.ok) {
			showMessage(registerMessage, result.message || 'Chưa thể tạo tài khoản. Vui lòng thử lại.');
			return;
		}

		showMessage(registerMessage, `Chào mừng ${result.user.username}! Hãy đăng nhập để bắt đầu.`, true);
		registerForm.reset();
	} catch {
		showMessage(registerMessage, 'Không kết nối được máy chủ. Hãy mở game qua máy chủ Virtual Pet rồi thử lại.');
	} finally {
		registerSubmit.disabled = false;
		registerSubmit.querySelector('span').textContent = 'Tạo tài khoản';
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
