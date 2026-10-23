let petLevel = 0;

function applyAccountState(state) {
	coins = state.user.coins;
	if (state.inventory) {
		inventory.splice(0, inventory.length, ...state.inventory.map((owned) => ({
			...shopItems.find((item) => item.id === owned.id), quantity: owned.quantity
		})));
	}
	if (state.pet) {
		document.querySelector('#pet-name').textContent = state.pet.name;
		document.querySelector('.pet-stage .pet-name').textContent = state.pet.name;
		const species = state.pet.species === 'dog' ? 'dog' : 'cat';
		const petImage = species === 'dog' ? 'assets/pets/gaugau.png' : 'assets/pets/meomeo.png';
		document.querySelector('.pet-stage .pet-image').src = petImage;
		document.querySelector('.pet-stage .pet-image').alt = `${species === 'dog' ? 'Dog' : 'Cat'}, ${state.pet.name}`;
		document.querySelector('.hud-avatar img').src = petImage;
		document.querySelector('#pet-level').textContent = state.pet.level;
		petLevel = state.pet.level;
		const exp = state.pet.exp ?? state.pet.experience ?? 0;
		const expMax = state.pet.level * 100;
		document.querySelector('#exp-text').textContent = `EXP ${exp} / ${expMax}`;
		document.querySelector('#exp-fill').style.width = `${Math.min(100, exp / expMax * 100)}%`;
		['happiness', 'hunger', 'energy'].forEach((name) => {
			const stat = document.querySelector(`[data-stat="${name}"]`);
			stat.querySelector('.hud-stat-value').textContent = `${state.pet[name]} / 100`;
			stat.querySelector('.px-bar i').style.width = `${state.pet[name]}%`;
		});
	}
	if (state.decor) {
		equippedDecor.background = state.decor.background ?? null;
		equippedDecor.floor = state.decor.floor ?? null;
	}
	accountReady = true;
	updateCoinDisplays();
	renderInventory();
	renderShop();
	renderFood();
	applyLobbyDecor();
	renderDecorate();
}

async function loadAccount() {
	const catalog = await api.listShop();
	shopItems.splice(0, shopItems.length, ...catalog);
	applyAccountState(await api.me());
}

function reportFailure(failure) {
	return failure.message || 'Unable to update the demo.';
}

document.querySelector('#open-food').addEventListener('click', () => {
	if (!accountReady) return;
	const drawer = document.querySelector('#food-drawer');
	drawer.hidden = !drawer.hidden;
	document.querySelector('#open-food').setAttribute('aria-expanded', String(!drawer.hidden));
	renderFood();
});

let feeding = false;
let selectedFood = null;
const petStage = document.querySelector('.pet-stage');
const feedingStatus = document.querySelector('#feeding-status');
petStage.tabIndex = 0;
petStage.setAttribute('role', 'button');
petStage.setAttribute('aria-label', 'Feed selected food to UIA');

document.querySelector('#close-food').addEventListener('click', () => {
	document.querySelector('#food-drawer').hidden = true;
	document.querySelector('#open-food').setAttribute('aria-expanded', 'false');
});

function renderFood() {
	const list = document.querySelector('#food-list');
	list.replaceChildren();
	const foods = inventory.filter((item) => item.category === 'food' && item.quantity > 0);
	if (!foods.length) {
		const empty = document.createElement('p');
		empty.textContent = 'No food in inventory';
		list.append(empty);
	}
	foods.forEach((item) => {
		const button = document.createElement('button');
		button.type = 'button';
		button.className = 'food-choice';
		button.disabled = feeding;
		button.setAttribute('aria-label', `${item.name}, ${item.quantity} remaining`);
		button.setAttribute('aria-pressed', String(selectedFood === item.id));
		const image = document.createElement('img');
		image.src = item.image;
		image.alt = '';
		image.draggable = false;
		const label = document.createElement('span');
		label.textContent = item.name;
		const count = document.createElement('b');
		count.textContent = `\u00d7${item.quantity}`;
		button.append(image, label, count);
		button.addEventListener('click', () => {
			selectedFood = item.id;
			list.querySelectorAll('button').forEach((choice) => choice.setAttribute('aria-pressed', String(choice === button)));
		});
		button.addEventListener('pointerdown', (event) => {
			if (feeding || event.button !== 0) return;
			const ghost = image.cloneNode();
			ghost.className = 'food-drag-ghost';
			document.body.append(ghost);
			button.setPointerCapture(event.pointerId);
			const move = (pointer) => {
				ghost.style.left = `${pointer.clientX}px`;
				ghost.style.top = `${pointer.clientY}px`;
				const bounds = petStage.getBoundingClientRect();
				petStage.classList.toggle('is-drop-target', pointer.clientX >= bounds.left && pointer.clientX <= bounds.right && pointer.clientY >= bounds.top && pointer.clientY <= bounds.bottom);
			};
			move(event);
			const finish = (pointer) => {
				const dropped = petStage.classList.contains('is-drop-target');
				ghost.remove();
				petStage.classList.remove('is-drop-target');
				button.removeEventListener('pointermove', move);
				button.removeEventListener('pointerup', finish);
				button.removeEventListener('pointercancel', finish);
				if (pointer.type === 'pointerup' && dropped) feedFood(item);
			};
			button.addEventListener('pointermove', move);
			button.addEventListener('pointerup', finish);
			button.addEventListener('pointercancel', finish);
		});
		list.append(button);
	});
}

async function feedFood(item) {
	if (feeding || !accountReady || !item) return;
	feeding = true;
	feedingStatus.textContent = '';
	renderFood();
	try {
		const previousLevel = petLevel;
		const fed = await api.feedPet(item.id);
		applyAccountState(fed);
		petStage.classList.remove('is-eating');
		void petStage.offsetWidth;
		petStage.classList.add('is-eating');
		const bite = document.createElement('img');
		bite.src = item.image;
		bite.alt = '';
		bite.className = 'eating-food';
		petStage.append(bite);
		bite.addEventListener('animationend', () => bite.remove(), { once: true });
		feedingStatus.textContent = fed.pet.level > previousLevel
			? `${item.name} eaten! Level up! UIA is now level ${fed.pet.level}`
			: `${item.name} eaten!`;
	} catch (failure) {
		feedingStatus.textContent = reportFailure(failure);
	} finally {
		feeding = false;
		renderFood();
	}
}

petStage.addEventListener('animationend', () => petStage.classList.remove('is-eating'));
petStage.addEventListener('click', () => feedFood(inventory.find((item) => item.id === selectedFood)));
petStage.addEventListener('keydown', (event) => {
	if (event.key === 'Enter' || event.key === ' ') {
		event.preventDefault();
		feedFood(inventory.find((item) => item.id === selectedFood));
	}
});

document.querySelector('#sleep-pet').addEventListener('click', async (event) => {
	if (!accountReady) return;
	const button = event.currentTarget;
	button.disabled = true;
	try {
		applyAccountState(await api.sleepPet());
		feedingStatus.textContent = 'UIA took a nap and feels rested!';
	} catch (failure) {
		feedingStatus.textContent = reportFailure(failure);
	} finally {
		button.disabled = false;
	}
});

const renameGate = document.querySelector('#rename-gate');
const renameForm = document.querySelector('#rename-form');
const renameInput = document.querySelector('#pet-new-name');
const renameError = document.querySelector('#rename-error');
document.querySelector('#rename-pet').addEventListener('click', () => {
	if (!accountReady) return;
	renameInput.value = document.querySelector('#pet-name').textContent;
	renameError.textContent = '';
	renameGate.hidden = false;
	renameInput.focus();
	renameInput.select();
});
document.querySelector('#cancel-rename').addEventListener('click', () => { renameGate.hidden = true; });
renameGate.addEventListener('click', (event) => {
	if (event.target === renameGate) renameGate.hidden = true;
});
renameForm.addEventListener('submit', async (event) => {
	event.preventDefault();
	const name = renameInput.value.trim();
	if (!name) { renameError.textContent = 'Enter a name for your pet.'; return; }
	const saveButton = document.querySelector('#save-pet-name');
	saveButton.disabled = true;
	renameError.textContent = '';
	try {
		applyAccountState(await api.savePet({ name }));
		renameGate.hidden = true;
	} catch (failure) {
		renameError.textContent = reportFailure(failure);
	} finally {
		saveButton.disabled = false;
	}
});

async function initializeAccount() {
	const gate = document.querySelector('#entry-gate');
	const form = document.querySelector('#entry-form');
	const actions = document.querySelector('#entry-actions');
	const error = document.querySelector('#entry-error');
	const submit = document.querySelector('#submit-auth');
	const petPicker = document.querySelector('#pet-picker');
	const petPickerError = document.querySelector('#pet-picker-error');
	let selectedSpecies = 'cat';
	let authMode = 'login';

	async function enterGame(selectedApi) {
		window.api = selectedApi;
		await loadAccount();
		gate.hidden = true;
		if (selectedApi === window.authApi) {
			selectedSpecies = document.querySelector('.pet-stage .pet-image').src.includes('gaugau.png') ? 'dog' : 'cat';
			document.querySelectorAll('[data-pet-species]').forEach((button) => {
				button.setAttribute('aria-pressed', String(button.dataset.petSpecies === selectedSpecies));
			});
			petPickerError.textContent = '';
			petPicker.hidden = false;
			return;
		}
		petPicker.hidden = true;
		document.body.classList.remove('entry-open');
	}

	function showAuth(mode) {
		authMode = mode;
		actions.hidden = true;
		form.hidden = false;
		error.textContent = '';
		submit.textContent = mode === 'register' ? 'Create account' : 'Log in';
		const passwordInput = document.querySelector('#entry-password');
		passwordInput.autocomplete = mode === 'register' ? 'new-password' : 'current-password';
		passwordInput.minLength = mode === 'register' ? 6 : 1;
		passwordInput.maxLength = 1024;
		document.querySelector('#entry-title').textContent = mode === 'register' ? 'Create account' : 'Log in';
		document.querySelector('#entry-email').focus();
	}

	document.body.classList.add('entry-open');
	document.querySelector('#show-login').addEventListener('click', () => showAuth('login'));
	document.querySelector('#show-register').addEventListener('click', () => showAuth('register'));
	document.querySelector('#toggle-password').addEventListener('click', (event) => {
		const button = event.currentTarget;
		const password = document.querySelector('#entry-password');
		const visible = password.type === 'password';
		password.type = visible ? 'text' : 'password';
		button.setAttribute('aria-pressed', String(visible));
		button.setAttribute('aria-label', visible ? 'Hide password' : 'Show password');
	});
	document.querySelector('#entry-back').addEventListener('click', () => {
		form.hidden = true;
		actions.hidden = false;
		error.textContent = '';
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
		petPickerError.textContent = '';
		try {
			applyAccountState(await api.choosePet(selectedSpecies));
			petPicker.hidden = true;
			document.body.classList.remove('entry-open');
		} catch (failure) {
			petPickerError.textContent = reportFailure(failure);
		} finally {
			button.disabled = false;
		}
	});
	form.addEventListener('submit', async (event) => {
		event.preventDefault();
		error.textContent = '';
		submit.disabled = true;
		try {
			const email = document.querySelector('#entry-email').value.trim();
			const password = document.querySelector('#entry-password').value;
			if (authMode === 'register') await window.authApi.register(email, password);
			else await window.authApi.login(email, password);
			await enterGame(window.authApi);
		} catch (failure) {
			error.textContent = reportFailure(failure);
		} finally {
			submit.disabled = false;
		}
	});

	if (window.authApi.auth.isLoggedIn) {
		try {
			await enterGame(window.authApi);
			return;
		} catch {
			window.authApi.auth.token = '';
		}
	}
	gate.hidden = false;
}

initializeAccount();
