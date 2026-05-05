import { init } from './app';
import './style.css';

const overlay = document.getElementById('loginOverlay')!;
const userInput = document.getElementById('loginUsername') as HTMLInputElement;
const passInput = document.getElementById('loginPassword') as HTMLInputElement;
const errorEl = document.getElementById('loginError')!;
const submitBtn = document.getElementById('loginBtn')!;

function doLogin() {
  const name = userInput.value.trim();
  if (!name) {
    errorEl.textContent = '请输入用户名';
    errorEl.style.display = 'block';
    return;
  }
  overlay.style.display = 'none';
  init(name);
}

submitBtn.addEventListener('click', doLogin);
passInput.addEventListener('keydown', e => { if (e.key === 'Enter') doLogin(); });
userInput.addEventListener('keydown', e => { if (e.key === 'Enter') passInput.focus(); });
