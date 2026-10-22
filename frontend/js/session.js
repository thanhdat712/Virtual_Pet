const exerciseDialog = document.querySelector('#exercise-dialog');
const exerciseMessage = document.querySelector('#exercise-message');
const exerciseList = document.querySelector('#exercise-list');
let accountProgress = [];
let petLevel = 0;

function applyAccountState(state) {
	coins = state.user.coins;
	if (state.inventory) {
		inventory.splice(0, inventory.length, ...state.inventory.map((owned) => ({
			...shopItems.find((item) => item.id === owned.id), quantity: owned.quantity
		})));
	}
	if (state.pet) {
		const profile = document.querySelectorAll('.player-card p strong');
		profile[0].textContent = state.pet.name;
		profile[1].textContent = state.pet.level;
		petLevel = state.pet.level;
		['happiness', 'hunger', 'energy'].forEach((name, index) => {
			const stat = document.querySelectorAll('.stat')[index];
			stat.querySelector('b').textContent = `${state.pet[name]}%`;
			stat.querySelector('.meter i').style.width = `${state.pet[name]}%`;
		});
	}
	if (state.progress) accountProgress = state.progress;
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

async function openExercises(mode) {
	if (!accountReady) return;
	document.querySelector('#exercise-title').textContent = mode;
	exerciseList.replaceChildren();
	exerciseMessage.textContent = 'Loading...';
	if (!exerciseDialog.open) exerciseDialog.showModal();
	try {
		const exercises = await api.listExercises();
		exerciseMessage.textContent = '';
		exercises.forEach((exercise) => {
			const form = document.createElement('form');
			form.className = 'exercise-row';
			const heading = document.createElement('h3');
			heading.textContent = exercise.title;
			const description = document.createElement('p');
			description.textContent = exercise.description;
			const progress = document.createElement('p');
			progress.className = 'exercise-progress';
			const updateProgress = () => {
				const saved = accountProgress.find((entry) => entry.exerciseId === exercise.id);
				progress.textContent = saved ? `${saved.status === 'completed' ? 'Completed' : 'In progress'} · Best: ${saved.bestScore}/${exercise.maxScore} · Attempts: ${saved.attempts}` : 'Not started';
			};
			updateProgress();
			const label = document.createElement('label');
			label.htmlFor = `answer-${exercise.id}`;
			label.textContent = 'Your answer';
			const answer = document.createElement('textarea');
			answer.id = label.htmlFor;
			answer.maxLength = 2048;
			answer.rows = 2;
			answer.required = true;
			const submit = document.createElement('button');
			submit.className = 'primary-button';
			submit.type = 'submit';
			submit.textContent = 'Check answer';
			const result = document.createElement('p');
			result.className = 'form-message';
			result.setAttribute('role', 'status');
			form.addEventListener('submit', async (event) => {
				event.preventDefault();
				submit.disabled = true;
				result.textContent = 'Checking...';
				try {
					const graded = await api.submitExercise(exercise.id, answer.value);
					accountProgress = accountProgress.filter((entry) => entry.exerciseId !== exercise.id);
					accountProgress.push(graded.progress);
					updateProgress();
					result.textContent = graded.score === 100 ? 'Correct! 100/100' : 'Not correct yet. 0/100';
				} catch (failure) {
					result.textContent = reportFailure(failure);
				} finally {
					submit.disabled = false;
				}
			});
			form.append(heading, description, progress, label, answer, submit, result);
			exerciseList.append(form);
		});
	} catch (failure) {
		exerciseMessage.textContent = reportFailure(failure);
	}
}

document.querySelector('.round-button.exercise').addEventListener('click', () => openExercises('Study'));
document.querySelector('.round-button.practice').addEventListener('click', () => openExercises('Practice'));
document.querySelector('#close-exercises').addEventListener('click', () => exerciseDialog.close());
document.querySelector('.action.feed').addEventListener('click', () => {
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

async function initializeAccount() {
	try { await loadAccount(); }
	catch (failure) { feedingStatus.textContent = reportFailure(failure); }
}

initializeAccount();