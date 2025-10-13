import argparse, csv

def read_rows(path):
    rows=[]; 
    with open(path, newline="", encoding="utf-8") as f:
        for i,r in enumerate(csv.reader(f)):
            if i==0: continue
            rows.append((int(r[0]), int(r[1]), int(r[2])))
    return rows

def confusion_and_metrics(labels, preds, max_class=4):
    y_true = {(c,t):y for c,t,y in labels}; y_pred = {(c,t):y for c,t,y in preds}
    keys = sorted(set(y_true.keys()) & set(y_pred.keys()))
    if not keys: raise ValueError("No overlapping (clip,tick) between labels and preds")
    K = max_class+1; cm=[[0]*K for _ in range(K)]; correct=0
    for k in keys:
        yt, yp = y_true[k], y_pred[k]
        if 0<=yt<=max_class and 0<=yp<=max_class: cm[yt][yp]+=1
        if yt==yp: correct+=1
    acc = correct/len(keys)
    prec=[]; rec=[]
    for c in range(K):
        tp=cm[c][c]; fp=sum(cm[r][c] for r in range(K))-tp; fn=sum(cm[c][r] for r in range(K))-tp
        p=tp/(tp+fp) if (tp+fp)>0 else 0.0; r=tp/(tp+fn) if (tp+fn)>0 else 0.0
        prec.append(p); rec.append(r)
    return cm, acc, prec, rec, len(keys)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--labels", required=True)
    ap.add_argument("--pred", required=True)
    ap.add_argument("--max-class", type=int, default=4)
    a=ap.parse_args()
    labels=read_rows(a.labels); preds=read_rows(a.pred)
    cm, acc, prec, rec, N = confusion_and_metrics(labels, preds, a.max_class)
    print(f"Samples: {N}"); print(f"Accuracy: {acc*100:.2f}%")
    print("Confusion (rows=true, cols=pred):")
    for row in cm: print("  " + " ".join(f"{v:6d}" for v in row))
    print("Precision:", ", ".join(f"{v:.3f}" for v in prec))
    print("Recall:   ", ", ".join(f"{v:.3f}" for v in rec))

if __name__ == "__main__":
    main()

