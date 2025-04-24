import React, { useEffect, useState } from 'react';
import { ref, onValue, set } from 'firebase/database';
import database from '../firebase';
import '../App.css';

import {
  Chart as ChartJS,
  BarElement,
  CategoryScale,
  LinearScale,
  Tooltip,
  Legend,
} from 'chart.js';
import { Bar } from 'react-chartjs-2';

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend);

/* ----------------------------- Constants ----------------------------- */
const SUBJECTS = ['Math', 'Science', 'Social Studies', 'English'];
const RAISE_HAND_CODE = 'E';
const ANSWER_CHOICES = ['A', 'B', 'C', 'D'];

/* ---------------------------------------------------------------------
   Format any epoch (s or ms) into "Apr 23 2025, 04:17:00 PM" (Chicago)
------------------------------------------------------------------------ */
const formatTimestamp = (raw) => {
  if (raw == null) return '-';
  const n = Number(raw);
  const ms = n > 1e12 ? n : n * 1000; // seconds ➜ ms
  return new Intl.DateTimeFormat('en-US', {
    timeZone: 'America/Chicago',
    year: 'numeric',
    month: 'short',
    day: '2-digit',
    hour: 'numeric',
    minute: '2-digit',
    second: '2-digit',
    hour12: true,
  }).format(new Date(ms));
};

/* =========================== React Component ========================= */
const StudentDashboard = () => {
  /* --------------------------- Local state --------------------------- */
  const studentId = 'student1';

  const [rfid, setRfid] = useState('');
  const [studentName, setStudentName] = useState('');
  const [emotionValue, setEmotionValue] = useState(5);
  const [latestResponse, setLatestResponse] = useState(null);
  const [lastRaiseHand, setLastRaiseHand] = useState(null);
  const [responseHistory, setResponseHistory] = useState([]);

  const [sharedPrompt, setSharedPrompt] = useState({
    question: '',
    subject: 'Math',
    correctAnswer: 'A',
  });

  // Teacher-side inputs
  const [teacherPromptInput, setTeacherPromptInput] = useState('');
  const [teacherSubject, setTeacherSubject] = useState('Math');
  const [teacherCorrectAnswer, setTeacherCorrectAnswer] = useState('A');

  /* ------------------------- Firebase listeners ---------------------- */
  useEffect(() => {
    const rfidRef = ref(database, `students/${studentId}/rfid`);
    const nameRef = ref(database, `students/${studentId}/name`);
    const promptRef = ref(database, 'currentPrompt');
    const responsesRef = ref(database, `students/${studentId}/responses`);

    // RFID + student name
    onValue(rfidRef, (snap) => setRfid(snap.val() || ''));
    onValue(nameRef, (snap) => setStudentName(snap.val() || ''));

    // Current prompt (shared with teacher controls)
    onValue(promptRef, (snap) => {
      const val = snap.val() || {};
      setSharedPrompt({
        question: val.question ?? '',
        subject: val.subject ?? 'Math',
        correctAnswer: val.correctAnswer ?? 'A',
      });
      setTeacherPromptInput(val.question ?? '');
      setTeacherSubject(val.subject ?? 'Math');
      setTeacherCorrectAnswer(val.correctAnswer ?? 'A');
    });

    // Response history + latest emotion + latest raise-hand
    onValue(responsesRef, (snap) => {
      const val = snap.val();
      if (!val) {
        setResponseHistory([]);
        setLatestResponse(null);
        setLastRaiseHand(null);
        setEmotionValue(5);
        return;
      }

      const entries = Object.entries(val).map(([id, data]) => ({ id, ...data }));
      entries.sort((a, b) => b.timestamp - a.timestamp);
      setResponseHistory(entries);

      // Latest overall event (could be raise-hand or answer)
      setLatestResponse(entries[0]);
      setEmotionValue(entries[0]?.emotion ?? 5);

      // Find the most recent raise-hand event, if any
      const raiseEntry = entries.find((e) => e.selectedAnswer === RAISE_HAND_CODE);
      setLastRaiseHand(raiseEntry || null);
    });
  }, []);

  /* ------------------------ Teacher prompt update -------------------- */
  const updatePrompt = () => {
    const promptRef = ref(database, 'currentPrompt');
    set(promptRef, {
      question: teacherPromptInput,
      subject: teacherSubject,
      correctAnswer: teacherCorrectAnswer,
    });
  };

  /* --------------------------- Chart helpers ------------------------- */
  const getCorrectnessChartData = () => {
    const data = SUBJECTS.map((subject) => {
      const attempts = responseHistory.filter(
        (r) => r.subject === subject && r.selectedAnswer !== RAISE_HAND_CODE
      );
      const total = attempts.length;
      const correct = attempts.filter((r) => r.isCorrect).length;
      return total ? (correct / total) * 100 : 0;
    });
    return {
      labels: SUBJECTS,
      datasets: [
        {
          label: 'Correctness Rate (%)',
          data,
          backgroundColor: 'rgba(75, 192, 192, 0.6)',
        },
      ],
    };
  };

  const getEmotionChartData = () => {
    const data = SUBJECTS.map((subject) => {
      const filtered = responseHistory.filter((r) => r.subject === subject);
      const total = filtered.length;
      return total ? filtered.reduce((sum, r) => sum + (r.emotion || 0), 0) / total : 0;
    });
    return {
      labels: SUBJECTS,
      datasets: [
        {
          label: 'Average Emotion',
          data,
          backgroundColor: 'rgba(255, 159, 64, 0.6)',
        },
      ],
    };
  };

  /* ----------------------------- Helpers ----------------------------- */
  const renderAnswerCell = (answer) =>
    answer === RAISE_HAND_CODE ? '🖐️ Raise Hand' : answer;

  /* ------------------------------ Render ----------------------------- */
  return (
    <div className="dashboard">
      <h2>Desk Dashboard</h2>

      {/* Student info */}
      <div className="section">
        <p className="info"><strong>RFID:</strong> {rfid || 'Waiting for scan...'}</p>
        <p className="info"><strong>Student:</strong> {studentName || '-'}</p>
      </div>

      {/* Live emotion bar */}
      <div className="section">
        <label><strong>Emotion Level:</strong></label>
        <div style={{ backgroundColor: '#eee', height: '10px', borderRadius: '5px', width: '100%' }}>
          <div
            style={{
              width: `${(emotionValue / 10) * 100}%`,
              backgroundColor: '#36a2eb',
              height: '100%',
              borderRadius: '5px',
            }}
          />
        </div>
        <p className="info">Value: {emotionValue}</p>
      </div>

      {/* Latest event section */}
      <div className="section">
        <label><strong>Latest Event:</strong></label>
        {latestResponse ? (
          latestResponse.selectedAnswer === RAISE_HAND_CODE ? (
            <p className="info" style={{ color: '#db8b00', fontWeight: 'bold' }}>
              🖐️ Student Raised Hand ({formatTimestamp(latestResponse.timestamp)})
            </p>
          ) : (
            <>
              <p className="info">Answer: <strong>{latestResponse.selectedAnswer}</strong></p>
              <p className="info">Correct Answer: <strong>{latestResponse.correctAnswer}</strong></p>
              <p className="info">Emotion: <strong>{latestResponse.emotion}</strong></p>
              <p className="info" style={{ color: latestResponse.isCorrect ? 'green' : 'red' }}>
                {latestResponse.isCorrect ? '✅ Correct' : '❌ Incorrect'}
              </p>
            </>
          )
        ) : (
          <p className="info">No events yet.</p>
        )}

        {/* Persistent raise-hand banner if the most recent raise-hand is NOT the latest event */}
        {lastRaiseHand && latestResponse?.id !== lastRaiseHand.id && (
          <p className="info" style={{ color: '#db8b00', fontWeight: 'bold' }}>
            🖐️ Student Raised Hand ({formatTimestamp(lastRaiseHand.timestamp)})
          </p>
        )}
      </div>

      {/* Current prompt */}
      <div className="section">
        <p className="info"><strong>Current Prompt:</strong> {sharedPrompt.question}</p>
        <p className="info"><strong>Subject:</strong> {sharedPrompt.subject}</p>
        <p className="info"><strong>Correct Answer:</strong> {sharedPrompt.correctAnswer}</p>
      </div>

      {/* Teacher controls */}
      <div className="section">
        <h3>Teacher: Set Prompt</h3>
        <label>Prompt:</label>
        <input
          type="text"
          value={teacherPromptInput}
          onChange={(e) => setTeacherPromptInput(e.target.value)}
        />

        <label>Subject:</label>
        <select value={teacherSubject} onChange={(e) => setTeacherSubject(e.target.value)}>
          {SUBJECTS.map((subj) => (
            <option key={subj} value={subj}>{subj}</option>
          ))}
        </select>

        <label>Correct Answer:</label>
        <select
          value={teacherCorrectAnswer}
          onChange={(e) => setTeacherCorrectAnswer(e.target.value)}
        >
          {ANSWER_CHOICES.map((ch) => (
            <option key={ch} value={ch}>{ch}</option>
          ))}
        </select>

        <button onClick={updatePrompt} style={{ marginTop: '10px' }}>
          Update Prompt
        </button>
      </div>

      {/* Response history */}
      <div className="section">
        <h3>Response History</h3>
        {responseHistory.length === 0 ? (
          <p>No responses yet.</p>
        ) : (
          <table style={{ width: '100%', borderCollapse: 'collapse' }}>
            <thead>
              <tr>
                <th>Time</th>
                <th>Question</th>
                <th>Subject</th>
                <th>Emotion</th>
                <th>Event</th>
                <th>Correct</th>
              </tr>
            </thead>
            <tbody>
              {responseHistory.map((r) => (
                <tr key={r.id}>
                  <td>{formatTimestamp(r.timestamp)}</td>
                  <td>{r.question}</td>
                  <td>{r.subject}</td>
                  <td>{r.emotion}</td>
                  <td>{renderAnswerCell(r.selectedAnswer)}</td>
                  <td style={{ color: r.isCorrect ? 'green' : 'red' }}>
                    {r.selectedAnswer === RAISE_HAND_CODE ? '-' : r.isCorrect ? '✅' : '❌'}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      {/* Charts */}
      <div className="section">
        <h3>Correctness by Subject</h3>
        <Bar
          data={getCorrectnessChartData()}
          options={{ responsive: true, scales: { y: { beginAtZero: true, max: 100 } } }}
        />
      </div>

      <div className="section">
        <h3>Average Emotion by Subject</h3>
        <Bar
          data={getEmotionChartData()}
          options={{ responsive: true, scales: { y: { beginAtZero: true, max: 10 } } }}
        />
      </div>
    </div>
  );
};

export default StudentDashboard;
